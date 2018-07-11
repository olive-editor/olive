#include "project.h"
#include "ui_project.h"
#include "io/media.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "playback/playback.h"
#include "effects/effects.h"
#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/effect.h"
#include "effects/transition.h"
#include "io/previewgenerator.h"

#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QCharRef>
#include <QMessageBox>
#include <QDropEvent>
#include <QMimeData>
#include <QPushButton>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

bool project_changed = false;
QString project_url = "";

Project::Project(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Project)
{
    ui->setupUi(this);
    source_table = ui->treeWidget;
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(rename_media(QTreeWidgetItem*,int)));
}

Project::~Project()
{
	delete ui;
}

QString Project::get_next_sequence_name() {
	int n = 1;
	bool found = true;
	QString name;
	while (found) {
		found = false;
		name = "Sequence ";
		if (n < 10) {
			name += "0";
		}
		name += QString::number(n);
		for (int i=0;i<ui->treeWidget->topLevelItemCount();i++) {
			if (QString::compare(ui->treeWidget->topLevelItem(i)->text(0), name, Qt::CaseInsensitive) == 0) {
				found = true;
				n++;
				break;
			}
		}
	}
	return name;
}

void Project::rename_media(QTreeWidgetItem* item, int column) {
    int type = get_type_from_tree(item);
    QString n = item->text(column);
    switch (type) {
    case MEDIA_TYPE_FOOTAGE: get_media_from_tree(item)->name = n; break;
    case MEDIA_TYPE_SEQUENCE: get_sequence_from_tree(item)->name = n; break;
    }
}

void Project::duplicate_selected() {
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    for (int j=0;j<items.size();j++) {
        QTreeWidgetItem* i = items.at(j);
        if (get_type_from_tree(i) == MEDIA_TYPE_SEQUENCE) {
            new_sequence(get_sequence_from_tree(i)->copy(), false, i->parent());
        }
    }
}

void Project::new_sequence(Sequence *s, bool open, QTreeWidgetItem* parent) {
    QTreeWidgetItem* item = new_item();
    item->setText(0, s->name);
    set_sequence_of_tree(item, s);

    if (parent == NULL) {
        ui->treeWidget->addTopLevelItem(item);
    } else {
        parent->addChild(item);
    }

    project_changed = true;

    if (open) set_sequence(s);
}

QTreeWidgetItem* Project::import_file(QString file) {
    QByteArray ba = file.toLatin1();
    char* filename = new char[ba.size()+1];
    strcpy(filename, ba.data());

    QTreeWidgetItem* item = NULL;

    AVFormatContext* pFormatCtx = NULL;
    int errCode = avformat_open_input(&pFormatCtx, filename, NULL, NULL);
    if(errCode != 0) {
        char err[1024];
        av_strerror(errCode, err, 1024);
        qDebug() << "[ERROR] Could not open" << filename << "-" << err;
    } else {
        errCode = avformat_find_stream_info(pFormatCtx, NULL);
        if (errCode < 0) {
            char err[1024];
            av_strerror(errCode, err, 1024);
            fprintf(stderr, "[ERROR] Could not find stream information. %s\n", err);
        } else {
            av_dump_format(pFormatCtx, 0, filename, 0);

            Media* m = new Media();
            m->url = file;
            m->name = file.mid(file.lastIndexOf('/')+1);

            // detect video/audio streams in file
            for (int i=0;i<(int)pFormatCtx->nb_streams;i++) {
                // Find the decoder for the video stream
                if (avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id) == NULL) {
                    qDebug() << "[ERROR] Unsupported codec in stream %d.\n";
                } else {
                    MediaStream* ms = new MediaStream();
                    ms->preview_done = false;
                    ms->file_index = i;
                    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                        bool infinite_length = (pFormatCtx->streams[i]->avg_frame_rate.den == 0);
                        ms->video_width = pFormatCtx->streams[i]->codecpar->width;
                        ms->video_height = pFormatCtx->streams[i]->codecpar->height;
                        ms->infinite_length = infinite_length;
                        m->video_tracks.append(ms);
                    } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                        ms->audio_channels = pFormatCtx->streams[i]->codecpar->channels;
                        m->audio_tracks.append(ms);
                    }
                }
            }
            m->length = pFormatCtx->duration;

            item = new_item();
            if (m->video_tracks.size() == 0) {
                item->setIcon(0, QIcon(":/icons/audiosource.png"));
            } else {
                item->setIcon(0, QIcon(":/icons/videosource.png"));
            }
            item->setText(0, m->name);
            item->setText(1, QString::number(m->length));
            set_media_of_tree(item, m);

            project_changed = true;

            // generate waveform/thumbnail in another thread
            PreviewGenerator* pg = new PreviewGenerator();
            pg->fmt_ctx = pFormatCtx; // cleaned up in PG
            pg->media = m;
            connect(pg, SIGNAL(finished()), pg, SLOT(deleteLater()));
            pg->start(QThread::LowPriority);
        }
    }

    delete [] filename;
    return item;
}

QTreeWidgetItem* Project::new_item() {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    return item;
}

bool Project::is_focused() {
    return ui->treeWidget->hasFocus();
}

QTreeWidgetItem* Project::new_folder() {
    QTreeWidgetItem* item = new_item();
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    item->setText(0, "New Folder");
    set_item_to_folder(item);
    ui->treeWidget->addTopLevelItem(item);
    project_changed = true;
    return item;
}

void Project::delete_selected_media() {
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    bool remove = true;
    bool redraw = false;

    // check if media is in use
    for (int i=0;i<items.size();i++) {
        QTreeWidgetItem* item = items.at(i);
        Media* m = get_media_from_tree(item);
        if (get_type_from_tree(item) == MEDIA_TYPE_FOOTAGE && sequence != NULL) {
            for (int j=0;j<sequence->clip_count();j++) {
                Clip* c = sequence->get_clip(j);
                if (c != NULL && c->media == m) {
                    remove = false;
                    break;
                }
            }
            if (!remove) {
                QMessageBox confirm(this);
                confirm.setWindowTitle("Delete media in use?");
                confirm.setText("The media '" + m->name + "' is currently used in the sequence. Deleting it will remove all instances in the sequence. Are you sure you want to do this?");
                QAbstractButton* yes_button = confirm.addButton(QMessageBox::Yes);
                QAbstractButton* skip_button = confirm.addButton("Skip", QMessageBox::NoRole);
                QAbstractButton* abort_button = confirm.addButton(QMessageBox::Abort);
                confirm.exec();
                if (confirm.clickedButton() == yes_button) {
                    // remove all clips referencing this media
                    for (int j=0;j<sequence->clip_count();j++) {
                        if (sequence->get_clip(j)->media == m) {
                            sequence->delete_clip(j);
                        }
                    }
                    remove = true;
                    redraw = true;
                } else if (confirm.clickedButton() == skip_button) {
                    items.removeAt(i);
                    i--;
                    remove = true;
                } else if (confirm.clickedButton() == abort_button) {
                    break;
                }
            }
        }
    }

    // remove
    if (remove) {
        for (int i=0;i<items.size();i++) {
            QTreeWidgetItem* item = items.at(i);
            delete_media(item);
            delete item;
        }
    }

    // redraw clips
    if (redraw) {
        panel_timeline->redraw_all_clips(true);
    }
}

void Project::process_file_list(QStringList& files) {
    for (int i=0;i<files.count();i++) {
        QString file(files.at(i));

        // heuristic to determine whether file is part of an image sequence
        int lastcharindex = file.lastIndexOf(".")-1;
        if (lastcharindex == -2) {
            lastcharindex = file.length() - 1;
        }
        if (file[lastcharindex].isDigit()) {
            bool is_img_sequence = false;
            QString sequence_test(file);
            char lastchar = sequence_test.at(lastcharindex).toLatin1();

            sequence_test[lastcharindex] = lastchar + 1;
            if (QFileInfo::exists(sequence_test)) is_img_sequence = true;

            sequence_test[lastcharindex] = lastchar - 1;
            if (QFileInfo::exists(sequence_test)) is_img_sequence = true;

            if (is_img_sequence &&
                    QMessageBox::question(this, "Image sequence detected", "The file '" + file + "' appears to be part of an image sequence. Would you like to import it as such?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
                // change url to an image sequence instead
                int digit_count = 0;
                QString ext = file.mid(lastcharindex+1);
                while (file[lastcharindex].isDigit()) {
                    digit_count++;
                    lastcharindex--;
                }
                QString new_filename = file.left(lastcharindex+1) + "%";
                if (digit_count < 10) {
                    new_filename += "0";
                }
                new_filename += QString::number(digit_count) + "d" + ext;
                file = new_filename;
            }
        }

        ui->treeWidget->addTopLevelItem(import_file(file));
    }
}

void Project::import_dialog() {
	QStringList files = QFileDialog::getOpenFileNames(this, "Import media...", "", "All Files (*.*)");
    process_file_list(files);
}

void Project::set_item_to_folder(QTreeWidgetItem* item) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_FOLDER);
}

Media* Project::get_media_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Media*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
}

void Project::set_media_of_tree(QTreeWidgetItem* item, Media* media) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_FOOTAGE);
    item->setData(0, Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(media)));
}

Sequence* Project::get_sequence_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Sequence*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
}

void Project::set_sequence_of_tree(QTreeWidgetItem* item, Sequence* sequence) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_SEQUENCE);
    item->setData(0, Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(sequence)));
}

int Project::get_type_from_tree(QTreeWidgetItem* item) {
    return item->data(0, Qt::UserRole + 1).toInt();
}

void Project::delete_media(QTreeWidgetItem* item) {
    int type = get_type_from_tree(item);
    switch (type) {
    case MEDIA_TYPE_FOOTAGE:
        delete get_media_from_tree(item);
        break;
    case MEDIA_TYPE_SEQUENCE:
        Sequence* s = get_sequence_from_tree(item);
        if (sequence == s) {
            set_sequence(NULL);
        }
        delete s;
        break;
    }

    project_changed = true;
}

void Project::clear() {
    while (ui->treeWidget->topLevelItemCount() > 0) {
        QTreeWidgetItem* item = ui->treeWidget->topLevelItem(0);
        delete_media(item);
        delete item;
    }
}

void Project::new_project() {
    // clear existing project
    set_sequence(NULL);
    clear();
    project_changed = false;
}

QTreeWidgetItem* Project::find_loaded_folder_by_id(int id) {
    for (int j=0;j<loaded_folders.size();j++) {
        QTreeWidgetItem* parent_item = loaded_folders.at(j);
        if (parent_item->data(0, Qt::UserRole + 3).toInt() == id) {
            return parent_item;
        }
    }
}

bool Project::load_worker(QFile& f, QXmlStreamReader& stream, int type) {
    f.seek(0);
    stream.setDevice(stream.device());

    QString root_search;
    QString child_search;

    switch (type) {
    case LOAD_TYPE_VERSION:
        root_search = "version";
        break;
    case MEDIA_TYPE_FOLDER:
        root_search = "folders";
        child_search = "folder";
        break;
    case MEDIA_TYPE_FOOTAGE:
        root_search = "media";
        child_search = "footage";
        break;
    case MEDIA_TYPE_SEQUENCE:
        root_search = "sequences";
        child_search = "sequence";
        break;
    }

    while (!stream.atEnd()) {
        stream.readNextStartElement();
        if (stream.name() == root_search) {
            if (type == LOAD_TYPE_VERSION) {
                if (stream.readElementText() != SAVE_VERSION) {
                    if (QMessageBox::warning(this, "Version Mismatch", "This project was saved in a different version of Olive and may not be fully compatible with this version. Would you like to attempt loading it anyway?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                        error_str = "Incompatible project version.";
                        return false;
                    }
                }
            } else {
                while (!(stream.name() == root_search && stream.isEndElement())) {
                    stream.readNext();
                    if (stream.name() == child_search && stream.isStartElement()) {
                        switch (type) {
                        case MEDIA_TYPE_FOLDER:
                        {
                            QTreeWidgetItem* folder = new_folder();
                            for (int j=0;j<stream.attributes().size();j++) {
                                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                if (attr.name() == "id") {
                                    folder->setData(0, Qt::UserRole + 3, attr.value().toInt());
                                } else if (attr.name() == "name") {
                                    folder->setText(0, attr.value().toString());
                                }
                            }
                            loaded_folders.append(folder);
                        }
                            break;
                        case MEDIA_TYPE_FOOTAGE:
                        {
                            QString name;
                            QString url;
                            int id;
                            int folder;
                            for (int j=0;j<stream.attributes().size();j++) {
                                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                if (attr.name() == "id") {
                                    id = attr.value().toInt();
                                } else if (attr.name() == "folder") {
                                    folder = attr.value().toInt();
                                } else if (attr.name() == "name") {
                                    name = attr.value().toString();
                                } else if (attr.name() == "url") {
                                    url = attr.value().toString();
                                }
                            }
                            QTreeWidgetItem* item = import_file(url);
                            Media* m = get_media_from_tree(item);
                            m->name = name;
                            item->setText(0, name);
                            m->save_id = id;
                            if (folder == 0) {
                                ui->treeWidget->addTopLevelItem(item);
                            } else {
                                find_loaded_folder_by_id(folder)->addChild(item);
                            }
                            loaded_media.append(m);
                        }
                            break;
                        case MEDIA_TYPE_SEQUENCE:
                        {
                            QTreeWidgetItem* parent = NULL;
                            Sequence* s = new Sequence();

                            // load attributes about stream
                            for (int j=0;j<stream.attributes().size();j++) {
                                const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                if (attr.name() == "name") {
                                    s->name = attr.value().toString();
                                } else if (attr.name() == "folder") {
                                    int folder = attr.value().toInt();
                                    if (folder > 0) parent = find_loaded_folder_by_id(folder);
                                } else if (attr.name() == "width") {
                                    s->width = attr.value().toInt();
                                } else if (attr.name() == "height") {
                                    s->height = attr.value().toInt();
                                } else if (attr.name() == "framerate") {
                                    s->frame_rate = attr.value().toFloat();
                                } else if (attr.name() == "afreq") {
                                    s->audio_frequency = attr.value().toInt();
                                } else if (attr.name() == "alayout") {
                                    s->audio_layout = attr.value().toInt();
                                }
                            }

                            // load all clips and clip information
                            while (!(stream.name() == child_search && stream.isEndElement()) && !stream.atEnd()) {
                                stream.readNextStartElement();
                                if (stream.name() == "clip" && stream.isStartElement()) {
                                    //<clip id="0" name="Brock_Drying_Pan.gif" clipin="0" in="44" out="144" track="-1" r="192" g="128" b="128" media="2" stream="0">
                                    int media_id, stream_id;
                                    Clip* c = new Clip();
                                    for (int j=0;j<stream.attributes().size();j++) {
                                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                        if (attr.name() == "name") {
                                            c->name = attr.value().toString();
                                        } else if (attr.name() == "id") {
                                            c->load_id = attr.value().toInt();
                                        } else if (attr.name() == "clipin") {
                                            c->clip_in = attr.value().toInt();
                                        } else if (attr.name() == "in") {
                                            c->timeline_in = attr.value().toInt();
                                        } else if (attr.name() == "out") {
                                            c->timeline_out = attr.value().toInt();
                                        } else if (attr.name() == "track") {
                                            c->track = attr.value().toInt();
                                        } else if (attr.name() == "r") {
                                            c->color_r = attr.value().toInt();
                                        } else if (attr.name() == "g") {
                                            c->color_g = attr.value().toInt();
                                        } else if (attr.name() == "b") {
                                            c->color_b = attr.value().toInt();
                                        } else if (attr.name() == "media") {
                                            media_id = attr.value().toInt();
                                        } else if (attr.name() == "stream") {
                                            stream_id = attr.value().toInt();
                                        }
                                    }

                                    // set media and media stream
                                    for (int j=0;j<loaded_media.size();j++) {
                                        Media* m = loaded_media.at(j);
                                        if (m->save_id == media_id) {
                                            c->media = m;
                                            c->media_stream = c->media->get_stream_from_file_index(stream_id);
                                            break;
                                        }
                                    }

                                    c->sequence = s;

                                    // load links and effects
                                    while (!(stream.name() == "clip" && stream.isEndElement()) && !stream.atEnd()) {
                                        stream.readNext();
                                        if (stream.name() == "linked" && stream.isStartElement()) {
                                            while (!(stream.name() == "linked" && stream.isEndElement()) && !stream.atEnd()) {
                                                stream.readNext();
                                                if (stream.name() == "link" && stream.isStartElement()) {
                                                    for (int k=0;k<stream.attributes().size();k++) {
                                                        const QXmlStreamAttribute& link_attr = stream.attributes().at(k);
                                                        if (link_attr.name() == "id") {
                                                            c->linked.append(link_attr.value().toInt());
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (stream.name() == "effects" && stream.isStartElement()) {
                                            while (!(stream.name() == "effects" && stream.isEndElement()) && !stream.atEnd()) {
                                                stream.readNext();
                                                if (stream.name() == "effect" && stream.isStartElement()) {
                                                    int effect_id = -1;
                                                    for (int j=0;j<stream.attributes().size();j++) {
                                                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                                        if (attr.name().toString() == QLatin1String("id")) {
                                                            effect_id = attr.value().toInt();
                                                            break;
                                                        }
                                                    }
                                                    if (effect_id != -1) {
                                                        Effect* e = create_effect(effect_id, c);
                                                        e->load(&stream);
                                                        c->effects.append(e);
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    s->add_clip(c);
                                }
                            }

                            // correct links and clip IDs
                            for (int i=0;i<s->clip_count();i++) {
                                // correct links
                                Clip* correct_clip = s->get_clip(i);
                                for (int j=0;j<correct_clip->linked.size();j++) {
                                    bool found = false;
                                    for (int k=0;k<s->clip_count();k++) {
                                        if (s->get_clip(k)->load_id == correct_clip->linked.at(j)) {
                                            correct_clip->linked[j] = k;
                                            found = true;
                                            break;
                                        }
                                    }
                                    if (!found) {
                                        correct_clip->linked.removeAt(j);
                                        j--;
                                        if (QMessageBox::warning(this, "Invalid Clip Link", "This project contains an invalid clip link. It may be corrupt. Would you like to continue loading it?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                                            delete s;
                                            return false;
                                        }
                                    }
                                }
                            }

                            new_sequence(s, false, parent);
                        }
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
    return true;
}

void Project::load_project() {
    new_project();

    QFile file(project_url);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[ERROR] Could not open file";
        return;
    }

    QXmlStreamReader stream(&file);

    bool cont = false;
    error_str.clear();

    // temp variables for loading
    loaded_folders.clear();
    loaded_media.clear();
    QVector<Media*> temp_media_list;
    QString temp_name;
    QString temp_url;
    int temp_media_id;
    Sequence* temp_seq;
    Clip* temp_clip;

    // find project file version
    cont = load_worker(file, stream, LOAD_TYPE_VERSION);

    // load folders first
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_FOLDER);
    }

    // load media
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_FOOTAGE);
    }

    // load sequences
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_SEQUENCE);
    }

    /*int state = LOAD_STATE_IDLE;
    while (!error && !stream.atEnd()) {
        stream.readNext();
        switch (state) {
        case LOAD_STATE_IDLE:
            if (stream.isStartElement()) {
                if (stream.name() == "footage") {
                    for (int j=0;j<stream.attributes().size();j++) {
                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                        if (attr.name() == "id") {
                            temp_media_id = attr.value().toInt();
                        }
                    }
                    state = LOAD_STATE_FOOTAGE;
                } else if (stream.name() == "sequence") {
                    temp_seq = new Sequence();
                    state = LOAD_STATE_SEQUENCE;
                }
            }
            break;
        case LOAD_STATE_FOOTAGE:
            if (stream.isEndElement() && stream.name() == "footage") {
                Media* m = import_file(temp_url);
                if (m == NULL) {
                    error = (QMessageBox::warning(this, "Source load error", "Couldn't load source file '" + temp_url + "'. This is unsupported and may lead to crashes. Do you wish to continue?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No);
                } else {
                    m->save_id = temp_media_id;
                    m->name = temp_name;
                    temp_media_list.append(m);
                }
                state = LOAD_STATE_IDLE;
            } else if (stream.isStartElement()) {
                if (stream.name() == "name") {
                    stream.readNext();
                    temp_name = stream.text().toString();
                } else if (stream.name() == "url") {
                    stream.readNext();
                    temp_url = stream.text().toString();
                }
            }
            break;
        case LOAD_STATE_SEQUENCE:
            if (stream.isEndElement() && stream.name() == "sequence") {
                // correct IDs
                for (int i=0;i<temp_seq->clip_count();i++) {
                    Clip* clip = temp_seq->get_clip(i);

                    // correct IDs in linked clips
                    for (int j=0;j<clip->linked.size();j++) {
                        bool found = false;
                        for (int k=0;k<temp_seq->clip_count();k++) {
                            if (temp_seq->get_clip(k)->load_id == clip->linked.at(j)) {
                                clip->linked[j] = k;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {                            
                            if (QMessageBox::warning(this, "Corrupt clip link", "Project contains a non-existent clip link. It may be corrupt. Would you like to load it anyway?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                                clip->linked.removeAt(j);
                            } else {
                                error = true;
                            }
                        }
                    }
                }

                temp_seq->reset_undo();
                new_sequence(temp_seq, false);
                state = LOAD_STATE_IDLE;
            } else if (stream.isStartElement()) {
                if (stream.name() == "name") {
                    stream.readNext();
                    temp_seq->name = stream.text().toString();
                } else if (stream.name() == "width") {
                    stream.readNext();
                    temp_seq->width = stream.text().toInt();
                } else if (stream.name() == "height") {
                    stream.readNext();
                    temp_seq->height = stream.text().toInt();
                } else if (stream.name() == "framerate") {
                    stream.readNext();
                    temp_seq->frame_rate = stream.text().toFloat();
                } else if (stream.name() == "afreq") {
                    stream.readNext();
                    temp_seq->audio_frequency = stream.text().toInt();
                } else if (stream.name() == "alayout") {
                    stream.readNext();
                    temp_seq->audio_layout = stream.text().toInt();
                } else if (stream.name() == "clip") {
                    temp_clip = new Clip();
                    temp_clip->sequence = temp_seq;
                    state = LOAD_STATE_CLIP;
                }
            }
            break;
        case LOAD_STATE_CLIP:
            if (stream.isEndElement() && stream.name() == "clip") {
                temp_seq->add_clip(temp_clip);
                state = LOAD_STATE_SEQUENCE;
            } else if (stream.isStartElement()) {
                if (stream.name() == "name") {
                    stream.readNext();
                    temp_clip->name = stream.text().toString();
                } else if (stream.name() == "id") {
                    stream.readNext();
                    temp_clip->load_id = stream.text().toInt();
                } else if (stream.name() == "clipin") {
                    stream.readNext();
                    temp_clip->clip_in = stream.text().toInt();
                } else if (stream.name() == "in") {
                    stream.readNext();
                    temp_clip->timeline_in = stream.text().toInt();
                } else if (stream.name() == "out") {
                    stream.readNext();
                    temp_clip->timeline_out = stream.text().toInt();
                } else if (stream.name() == "track") {
                    stream.readNext();
                    temp_clip->track = stream.text().toInt();
                } else if (stream.name() == "opening") {
                    stream.readNext();
                    temp_clip->opening_transition = create_transition(stream.text().toInt(), temp_clip);
                } else if (stream.name() == "closing") {
                    stream.readNext();
                    temp_clip->closing_transition = create_transition(stream.text().toInt(), temp_clip);
                } else if (stream.name() == "color") {
                    for (int j=0;j<stream.attributes().size();j++) {
                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                        if (attr.name().toString() == QLatin1String("r")) {
                            temp_clip->color_r = attr.value().toInt();
                        } else if (attr.name().toString() == QLatin1String("g")) {
                            temp_clip->color_g = attr.value().toInt();
                        } else if (attr.name().toString() == QLatin1String("b")) {
                            temp_clip->color_b = attr.value().toInt();
                        }
                    }
                } else if (stream.name() == "media") {
                    stream.readNext();
                    for (int i=0;i<temp_media_list.size();i++) {
                        Media* m = temp_media_list.at(i);
                        if (m->save_id == stream.text().toInt()) {
                            temp_clip->media = m;
                            break;
                        }
                    }
                } else if (stream.name() == "stream") {
                    stream.readNext();
                    int stream_index = stream.text().toInt();
                    if (temp_clip->media != NULL) {
                        temp_clip->media_stream = temp_clip->media->get_stream_from_file_index(stream_index);
                        if (temp_clip->media_stream == NULL) {
                            if (QMessageBox::warning(this, "Source changed", "Source file '" + temp_clip->media->url + "' appears to have changed since last saving the project. This is unsupported and may lead to crashes. Do you wish to continue?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                                error = true;
                            }
                        }
                    }
                } else if (stream.name() == "linked") {
                    state = LOAD_STATE_CLIP_LINKS;
                } else if (stream.name() == "effect") {
                    int effect_id = -1;
                    for (int j=0;j<stream.attributes().size();j++) {
                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                        if (attr.name().toString() == QLatin1String("id")) {
                            effect_id = attr.value().toInt();
                        }
                    }
                    if (effect_id != -1) {
                        Effect* e = create_effect(effect_id, temp_clip);
                        e->load(&stream);
                        temp_clip->effects.append(e);
                    }
                }
            }
            break;
        case LOAD_STATE_CLIP_LINKS:
            if (stream.name() == "linked" && stream.isEndElement()) {
                state = LOAD_STATE_CLIP;
            } else if (stream.isStartElement() && stream.name() == "link") {
                stream.readNext();
                temp_clip->linked.append(stream.text().toInt());
            }
            break;
        }
    }*/
    if (!cont) {
        QMessageBox::critical(this, "Project Load Error", "Error loading project: " + error_str, QMessageBox::Ok);
    } else if (stream.hasError()) {
        qDebug() << "[ERROR] Error parsing XML." << stream.errorString();
        QMessageBox::critical(this, "XML Parsing Error", "Couldn't load '" + project_url + "'. " + stream.errorString(), QMessageBox::Ok);
        cont = false;
    }

    if (cont) {
        panel_timeline->redraw_all_clips(false);
        project_changed = false;
    } else {
        new_project();
    }
}

void Project::save_folder(QXmlStreamWriter& stream, QTreeWidgetItem* parent, int type) {
    bool root = (parent == NULL);
    int len = root ? ui->treeWidget->topLevelItemCount() : parent->childCount();
    for (int i=0;i<len;i++) {
        QTreeWidgetItem* item = root ? ui->treeWidget->topLevelItem(i) : parent->child(i);
        int item_type = get_type_from_tree(item);
        if (item_type == MEDIA_TYPE_FOLDER) {
            if (type == MEDIA_TYPE_FOLDER) {
                // if we're saving folders, save the folder
                stream.writeStartElement("folder");
                stream.writeAttribute("name", item->text(0));
                stream.writeAttribute("id", QString::number(folder_id));
                item->setData(0, Qt::UserRole + 3, folder_id); // saves a temporary ID for matching in the project file
                stream.writeEndElement();
                folder_id++;
            }
            save_folder(stream, item, type);
        } else if (type == item_type) {
            int folder = root ? 0 : parent->data(0, Qt::UserRole + 3).toInt();
            if (type == MEDIA_TYPE_FOOTAGE) {
                Media* m = get_media_from_tree(item);
                m->save_id = media_id;
                stream.writeStartElement("footage");
                stream.writeAttribute("id", QString::number(media_id));
                stream.writeAttribute("folder", QString::number(folder));
                stream.writeAttribute("name", m->name);
                stream.writeAttribute("url", m->url);
                stream.writeEndElement();
                media_id++;
            } else if (type == MEDIA_TYPE_SEQUENCE) {
                Sequence* s = get_sequence_from_tree(item);
                stream.writeStartElement("sequence");
                stream.writeAttribute("folder", QString::number(folder));
                stream.writeAttribute("name", s->name);
                stream.writeAttribute("width", QString::number(s->width));
                stream.writeAttribute("height", QString::number(s->height));
                stream.writeAttribute("framerate", QString::number(s->frame_rate));
                stream.writeAttribute("afreq", QString::number(s->audio_frequency));
                stream.writeAttribute("alayout", QString::number(s->audio_layout));
                for (int j=0;j<s->clip_count();j++) {
                    Clip* c = s->get_clip(j);
                    if (c != NULL) {
                        stream.writeStartElement("clip"); // clip
                        stream.writeAttribute("id", QString::number(j));
                        stream.writeAttribute("name", c->name);
                        stream.writeAttribute("clipin", QString::number(c->clip_in));
                        stream.writeAttribute("in", QString::number(c->timeline_in));
                        stream.writeAttribute("out", QString::number(c->timeline_out));
                        stream.writeAttribute("track", QString::number(c->track));
                        if (c->opening_transition != NULL) {
                            stream.writeAttribute("opening", QString::number(c->opening_transition->id));
                        }
                        if (c->closing_transition != NULL) {
                            stream.writeAttribute("closing", QString::number(c->closing_transition->id));
                        }
                        stream.writeAttribute("r", QString::number(c->color_r));
                        stream.writeAttribute("g", QString::number(c->color_g));
                        stream.writeAttribute("b", QString::number(c->color_b));
                        stream.writeAttribute("media", QString::number(c->media->save_id));
                        stream.writeAttribute("stream", QString::number(c->media_stream->file_index));

                        stream.writeStartElement("linked"); // linked
                        for (int k=0;k<c->linked.size();k++) {
                            stream.writeStartElement("link"); // link
                            stream.writeAttribute("id", QString::number(c->linked.at(k)));
                            stream.writeEndElement(); // link
                        }
                        stream.writeEndElement(); // linked

                        stream.writeStartElement("effects"); // effects
                        for (int k=0;k<c->effects.size();k++) {
                            stream.writeStartElement("effect"); // effect
                            Effect* e = c->effects.at(k);
                            stream.writeAttribute("id", QString::number(e->id));
                            e->save(&stream);
                            stream.writeEndElement(); // effect
                        }
                        stream.writeEndElement(); // effects
                        stream.writeEndElement(); // clip
                    }
                }
                stream.writeEndElement();
            }
        }
    }
}

void Project::save_project() {
    folder_id = 1;
    media_id = 0;

    QFile file(project_url);
    if (!file.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
        qDebug() << "[ERROR] Could not open file";
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument(); // doc

    stream.writeStartElement("project"); // project

    stream.writeTextElement("version", SAVE_VERSION);

    stream.writeStartElement("folders"); // folders
    save_folder(stream, NULL, MEDIA_TYPE_FOLDER);
    stream.writeEndElement(); // folders

    stream.writeStartElement("media"); // media
    save_folder(stream, NULL, MEDIA_TYPE_FOOTAGE);
    stream.writeEndElement(); // media

    stream.writeStartElement("sequences"); // sequences
    save_folder(stream, NULL, MEDIA_TYPE_SEQUENCE);
    stream.writeEndElement();// sequences

    stream.writeEndElement(); // project

    stream.writeEndDocument(); // doc

    file.close();

    project_changed = false;
}
