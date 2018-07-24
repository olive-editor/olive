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
#include "project/undo.h"
#include "mainwindow.h"
#include "io/config.h"

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

#define MAXIMUM_RECENT_PROJECTS 10

QString autorecovery_filename;
bool project_changed = false;
QString project_url = "";
QStringList recent_projects;
QString recent_proj_file;

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
    bool duped = false;
    TimelineAction* ta = new TimelineAction();
    for (int j=0;j<items.size();j++) {
        QTreeWidgetItem* i = items.at(j);
        if (get_type_from_tree(i) == MEDIA_TYPE_SEQUENCE) {
            new_sequence(ta, get_sequence_from_tree(i)->copy(), false, i->parent());
            duped = true;
        }
    }
    if (duped) {
        undo_stack.push(ta);
    } else {
        delete ta;
    }
}

void Project::new_sequence(TimelineAction *ta, Sequence *s, bool open, QTreeWidgetItem* parent) {
    QTreeWidgetItem* item = new_item();
    item->setText(0, s->name);
    set_sequence_of_tree(item, s);

    if (ta != NULL) {
        ta->new_sequence(item, parent);
        if (open) ta->change_sequence(s);
    } else {
        if (parent == NULL) {
            ui->treeWidget->addTopLevelItem(item);
        } else {
            parent->addChild(item);
        }
        if (open) set_sequence(s);
    }

    project_changed = true;
}

void Project::start_preview_generator(QTreeWidgetItem* item, Media* media) {
    // set up throbber animation
    MediaThrobber* throbber = new MediaThrobber(item);

    PreviewGenerator* pg = new PreviewGenerator(item, media);
    connect(pg, SIGNAL(set_icon(int)), throbber, SLOT(stop(int)));
    connect(pg, SIGNAL(finished()), pg, SLOT(deleteLater()));
    pg->start(QThread::LowPriority);
}

QTreeWidgetItem* Project::import_file(QString file, QString imported_filename) {
    QTreeWidgetItem* item = new_item();
    Media* m = new Media();
    m->url = file;
    m->name = imported_filename.mid(file.lastIndexOf('/')+1);
    set_media_of_tree(item, m);

    // generate waveform/thumbnail in another thread
    start_preview_generator(item, m);

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

void Project::get_media_from_table(QList<QTreeWidgetItem*> items, QList<QTreeWidgetItem*>& list, int search_type) {
    for (int i=0;i<items.size();i++) {
        QTreeWidgetItem* item = items.at(i);
        int type = get_type_from_tree(item);
        if (type == MEDIA_TYPE_FOLDER) {
            QList<QTreeWidgetItem*> children;
            for (int j=0;j<item->childCount();j++) {
                children.append(item->child(j));
            }
            get_media_from_table(children, list, search_type);
        } else if (search_type == type) {
            list.append(item);
        }
    }
}

void Project::delete_selected_media() {
    TimelineAction* ta = new TimelineAction();
    QList<QTreeWidgetItem*> items = ui->treeWidget->selectedItems();
    bool remove = true;
    bool redraw = false;

    // check if media is in use
    QVector<QTreeWidgetItem*> parents;
    QList<QTreeWidgetItem*> sequence_items;
    QList<QTreeWidgetItem*> all_top_level_items;
    for (int i=0;i<ui->treeWidget->topLevelItemCount();i++) {
        all_top_level_items.append(ui->treeWidget->topLevelItem(i));
    }
    get_media_from_table(all_top_level_items, sequence_items, MEDIA_TYPE_SEQUENCE); // find all sequences in project
    if (sequence_items.size() > 0) {
        QList<QTreeWidgetItem*> media_items;
        get_media_from_table(items, media_items, MEDIA_TYPE_FOOTAGE);
        for (int i=0;i<media_items.size();i++) {
            QTreeWidgetItem* item = media_items.at(i);
            Media* media = get_media_from_tree(item);
            bool confirm_delete = false;
            for (int j=0;j<sequence_items.size();j++) {
                Sequence* s = get_sequence_from_tree(sequence_items.at(j));
                for (int k=0;k<s->clip_count();k++) {
                    Clip* c = s->get_clip(k);
                    if (c != NULL && c->media == media) {
                        if (!confirm_delete) {
                            // we found a reference, so we know we'll need to ask if the user wants to delete it
                            QMessageBox confirm(this);
                            confirm.setWindowTitle("Delete media in use?");
                            confirm.setText("The media '" + media->name + "' is currently used in '" + s->name + "'. Deleting it will remove all instances in the sequence. Are you sure you want to do this?");
                            QAbstractButton* yes_button = confirm.addButton(QMessageBox::Yes);
                            QAbstractButton* skip_button = NULL;
                            if (items.size() > 1) skip_button = confirm.addButton("Skip", QMessageBox::NoRole);
                            QAbstractButton* abort_button = confirm.addButton(QMessageBox::Cancel);
                            confirm.exec();
                            if (confirm.clickedButton() == yes_button) {
                                // remove all clips referencing this media
                                confirm_delete = true;
                                redraw = true;
                            } else if (confirm.clickedButton() == skip_button) {
                                // remove media item and any folders containing it from the remove list
                                QTreeWidgetItem* parent = item;
                                while (parent != NULL) {
                                    parents.append(parent);

                                    // re-add item's siblings
                                    for (int m=0;m<parent->childCount();m++) {
                                        QTreeWidgetItem* child = parent->child(m);
                                        bool found = false;
                                        for (int n=0;n<items.size();n++) {
                                            if (items.at(n) == child) {
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found) {
                                            items.append(child);
                                        }
                                    }

                                    parent = parent->parent();
                                }

                                j = sequence_items.size();
                                k = s->clip_count();
                            } else if (confirm.clickedButton() == abort_button) {
                                // break out of loop
                                i = media_items.size();
                                j = sequence_items.size();
                                k = s->clip_count();

                                remove = false;
                            }
                        }
                        if (confirm_delete) {
                            ta->delete_clip(s, k);
                        }
                    }
                }
            }
        }
    }

    // remove
    if (remove) {
        // remove media and parents
        for (int m=0;m<parents.size();m++) {
            for (int l=0;l<items.size();l++) {
                if (items.at(l) == parents.at(m)) {
                    items.removeAt(l);
                    l--;
                }
            }
        }

        for (int i=0;i<items.size();i++) {
            // send delete command to the TimelineAction
            if (get_type_from_tree(items.at(i)) == MEDIA_TYPE_SEQUENCE) {
                redraw = true;
            }
            ta->delete_media(items.at(i));
        }
        undo_stack.push(ta);

        // redraw clips
        if (redraw) {
            panel_timeline->redraw_all_clips(true);
        }

        project_changed = true;
    } else {
        delete ta;
    }
}

void Project::process_file_list(QStringList& files) {
    bool imported = false;

    QVector<QString> image_sequence_urls;
    QVector<bool> image_sequence_importassequence;
    QStringList image_sequence_formats = config.img_seq_formats.split("|");

    TimelineAction* ta = new TimelineAction();
    for (int i=0;i<files.count();i++) {
        QString file(files.at(i));
        bool skip = false;

        /* Heuristic to determine whether file is part of an image sequence */

        // check file extension (assume it's not a
        int lastcharindex = file.lastIndexOf(".");
        bool found = true;
        if (lastcharindex != -1 && lastcharindex > file.lastIndexOf('/')) {
            // image_sequence_formats
            found = false;
            QString ext = file.mid(lastcharindex+1);
            for (int j=0;j<image_sequence_formats.size();j++) {
                if (ext == image_sequence_formats.at(j)) {
                    found = true;
                    break;
                }
            }
        } else {
            lastcharindex = file.length();
        }

        if (found && file[lastcharindex-1].isDigit()) {
            bool is_img_sequence = false;

            // how many digits are in the filename?
            int digit_count = 0;
            int digit_test = lastcharindex-1;
            while (file[digit_test].isDigit()) {
                digit_count++;
                digit_test--;
            }

            // retrieve number from filename
            digit_test++;
            int file_number = file.mid(digit_test, digit_count).toInt();

            // Check if there are files with the same filename but just different numbers
            if (QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number-1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))
                    || QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number+1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))) {
                is_img_sequence = true;
            }

            if (is_img_sequence) {
                // get the URL that we would pass to FFmpeg to force it to read the image as a sequence
                QString new_filename = file.left(digit_test) + "%" + QString("%1").arg(digit_count, 2, 10, QChar('0')) + "d" + file.mid(lastcharindex);

                // add image sequence url to a vector in case the user imported several files that
                // we're interpreting as a possible sequence
                found = false;
                for (int i=0;i<image_sequence_urls.size();i++) {
                    if (image_sequence_urls.at(i) == new_filename) {
                        // either SKIP if we're importing as a sequence, or leave it if we aren't
                        if (image_sequence_importassequence.at(i)) {
                            skip = true;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    image_sequence_urls.append(new_filename);
                    if (QMessageBox::question(this, "Image sequence detected", "The file '" + file + "' appears to be part of an image sequence. Would you like to import it as such?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
                        file = new_filename;
                        image_sequence_importassequence.append(true);
                    } else {
                        image_sequence_importassequence.append(false);
                    }
                }
            }
        }        

        if (!skip) {
            ta->add_media(import_file(file, files.at(i)));
            imported = true;
            project_changed = true;
        }
    }
    if (imported) {
        undo_stack.push(ta);
    } else {
        delete ta;
    }
}

void Project::import_dialog() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Import media...", "", "All Files (*)");
    process_file_list(files);
}

void set_item_to_folder(QTreeWidgetItem* item) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_FOLDER);
}

Media* get_media_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Media*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
}

void set_media_of_tree(QTreeWidgetItem* item, Media* media) {
    item->setText(0, media->name);
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_FOOTAGE);
    item->setData(0, Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(media)));
}

Sequence* get_sequence_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Sequence*>(item->data(0, Qt::UserRole + 2).value<quintptr>());
}

void set_sequence_of_tree(QTreeWidgetItem* item, Sequence* sequence) {
    item->setData(0, Qt::UserRole + 1, MEDIA_TYPE_SEQUENCE);
    item->setData(0, Qt::UserRole + 2, QVariant::fromValue(reinterpret_cast<quintptr>(sequence)));
}

int get_type_from_tree(QTreeWidgetItem* item) {
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
    return NULL;
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
                                } else if (attr.name() == "parent") {
                                    folder->setData(0, Qt::UserRole + 4, attr.value().toInt());
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
                            QTreeWidgetItem* item = new_item();//import_file(url);
                            Media* m = new Media();
                            m->url = url;
                            m->name = name;
                            m->save_id = id;

                            while (!(stream.name() == child_search && stream.isEndElement())) {
                                stream.readNext();
                                if (stream.isStartElement()) {
                                    if (stream.name() == "video") {
                                        MediaStream* ms = new MediaStream();
                                        for (int j=0;j<stream.attributes().size();j++) {
                                            const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                            if (attr.name() == "id") {
                                                ms->file_index = attr.value().toInt();
                                            } else if (attr.name() == "width") {
                                                ms->video_width = attr.value().toInt();
                                            } else if (attr.name() == "height") {
                                                ms->video_height = attr.value().toInt();
                                            } else if (attr.name() == "framerate") {
                                                ms->video_frame_rate = attr.value().toDouble();
                                            } else if (attr.name() == "infinite") {
                                                ms->infinite_length = (attr.value().toInt() == 1);
                                            }
                                        }
                                        m->video_tracks.append(ms);
                                    } else if (stream.name() == "audio") {
                                        MediaStream* ms = new MediaStream();
                                        for (int j=0;j<stream.attributes().size();j++) {
                                            const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                            if (attr.name() == "id") {
                                                ms->file_index = attr.value().toInt();
                                            } else if (attr.name() == "channels") {
                                                ms->audio_channels = attr.value().toInt();
                                            } else if (attr.name() == "layout") {
                                                ms->audio_layout = attr.value().toInt();
                                            } else if (attr.name() == "frequency") {
                                                ms->audio_frequency = attr.value().toInt();
                                            }
                                        }
                                        m->audio_tracks.append(ms);
                                    }
                                }
                            }

                            set_media_of_tree(item, m);
                            if (folder == 0) {
                                ui->treeWidget->addTopLevelItem(item);
                            } else {
                                find_loaded_folder_by_id(folder)->addChild(item);
                            }

                            // analyze media to see if it's the same
                            start_preview_generator(item, m);

                            loaded_media.append(m);
                        }
                            break;
                        case MEDIA_TYPE_SEQUENCE:
                        {
                            QTreeWidgetItem* parent = NULL;
                            Sequence* s = new Sequence();
                            bool open_sequence = false;

                            // load attributes about sequence
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
                                } else if (attr.name() == "open") {
                                    open_sequence = true;
                                } else if (attr.name() == "workareaIn") {
                                    s->using_workarea = true;
                                    s->workarea_in = attr.value().toLong();
                                } else if (attr.name() == "workareaOut") {
                                    s->workarea_out = attr.value().toLong();
                                }
                            }

                            // load all clips and clip information
                            while (!(stream.name() == child_search && stream.isEndElement()) && !stream.atEnd()) {
                                stream.readNextStartElement();
                                if (stream.name() == "clip" && stream.isStartElement()) {
                                    //<clip id="0" name="Brock_Drying_Pan.gif" clipin="0" in="44" out="144" track="-1" r="192" g="128" b="128" media="2" stream="0">
                                    int media_id, stream_id;
                                    Clip* c = new Clip(s);
                                    for (int j=0;j<stream.attributes().size();j++) {
                                        const QXmlStreamAttribute& attr = stream.attributes().at(j);
                                        if (attr.name() == "name") {
                                            c->name = attr.value().toString();
                                        } else if (attr.name() == "id") {
                                            c->load_id = attr.value().toInt();
                                        } else if (attr.name() == "clipin") {
                                            c->clip_in = attr.value().toLong();
                                        } else if (attr.name() == "in") {
                                            c->timeline_in = attr.value().toLong();
                                        } else if (attr.name() == "out") {
                                            c->timeline_out = attr.value().toLong();
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
                                            c->media_stream = stream_id;
                                            break;
                                        }
                                    }

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

                            new_sequence(NULL, s, false, parent);

                            if (open_sequence) set_sequence(s);
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

    // find project file version
    cont = load_worker(file, stream, LOAD_TYPE_VERSION);

    // load folders first
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_FOLDER);
    }

    // load media
    if (cont) {
        // since folders loaded correctly, organize them appropriately
        for (int i=0;i<loaded_folders.size();i++) {
            QTreeWidgetItem* folder = loaded_folders.at(i);
            int parent = folder->data(0, Qt::UserRole + 4).toInt();
            if (parent > 0) {
                ui->treeWidget->takeTopLevelItem(ui->treeWidget->indexOfTopLevelItem(folder));
                find_loaded_folder_by_id(parent)->addChild(folder);
            }
        }

        cont = load_worker(file, stream, MEDIA_TYPE_FOOTAGE);
    }

    // load sequences
    if (cont) {
        cont = load_worker(file, stream, MEDIA_TYPE_SEQUENCE);
    }

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

    add_recent_project(project_url);

    file.close();
}

void Project::save_folder(QXmlStreamWriter& stream, QTreeWidgetItem* parent, int type) {
    bool root = (parent == NULL);
    int len = root ? ui->treeWidget->topLevelItemCount() : parent->childCount();
    for (int i=0;i<len;i++) {
        QTreeWidgetItem* item = root ? ui->treeWidget->topLevelItem(i) : parent->child(i);
        int item_type = get_type_from_tree(item);
        if (item_type == MEDIA_TYPE_FOLDER) {
            if (type == SAVE_SET_FOLDER_IDS) {
                item->setData(0, Qt::UserRole + 3, folder_id); // saves a temporary ID for matching in the project file
                folder_id++;
            } else if (type == MEDIA_TYPE_FOLDER) {
                // if we're saving folders, save the folder
                stream.writeStartElement("folder");
                stream.writeAttribute("name", item->text(0));
                stream.writeAttribute("id", QString::number(item->data(0, Qt::UserRole + 3).toInt()));
                if (item->parent() == NULL) {
                    stream.writeAttribute("parent", "0");
                } else {
                    stream.writeAttribute("parent", QString::number(item->parent()->data(0, Qt::UserRole + 3).toInt()));
                }
                stream.writeEndElement();                
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
                stream.writeAttribute("duration", QString::number(m->length));
                for (int j=0;j<m->video_tracks.size();j++) {
                    MediaStream* ms = m->video_tracks.at(j);
                    stream.writeStartElement("video");
                    stream.writeAttribute("id", QString::number(ms->file_index));
                    stream.writeAttribute("width", QString::number(ms->video_width));
                    stream.writeAttribute("height", QString::number(ms->video_height));
                    stream.writeAttribute("framerate", QString::number(ms->video_frame_rate));
                    stream.writeAttribute("infinite", QString::number(ms->infinite_length));
                    stream.writeEndElement();
                }
                for (int j=0;j<m->audio_tracks.size();j++) {
                    MediaStream* ms = m->audio_tracks.at(j);
                    stream.writeStartElement("audio");
                    stream.writeAttribute("id", QString::number(ms->file_index));
                    stream.writeAttribute("channels", QString::number(ms->audio_channels));
                    stream.writeAttribute("layout", QString::number(ms->audio_layout));
                    stream.writeAttribute("frequency", QString::number(ms->audio_frequency));
                    stream.writeEndElement();
                }
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
                if (s == sequence) {
                    stream.writeAttribute("open", "1");
                }
                if (s->using_workarea) {
                    stream.writeAttribute("workareaIn", QString::number(s->workarea_in));
                    stream.writeAttribute("workareaOut", QString::number(s->workarea_out));
                }
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
                        stream.writeAttribute("stream", QString::number(c->media_stream));

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

void Project::save_project(bool autorecovery) {
    folder_id = 1;
    media_id = 0;

    QFile file(autorecovery ? autorecovery_filename : project_url);
    if (!file.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
        qDebug() << "[ERROR] Could not open file";
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument(); // doc

    stream.writeStartElement("project"); // project

    stream.writeTextElement("version", SAVE_VERSION);

    save_folder(stream, NULL, SAVE_SET_FOLDER_IDS);

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

    if (!autorecovery) {
        add_recent_project(project_url);
        project_changed = false;
    }
}

void Project::save_recent_projects() {
    // save to file
    QFile f(recent_proj_file);
    if (f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
        QTextStream out(&f);
        for (int i=0;i<recent_projects.size();i++) {
            if (i > 0) {
                out << "\n";
            }
            out << recent_projects.at(i);
        }
        f.close();
    } else {
        qDebug() << "[WARNING] Could not save recent projects";
    }
}

void Project::clear_recent_projects() {
    recent_projects.clear();
    save_recent_projects();
}

void Project::add_recent_project(QString url) {
    bool found = false;
    for (int i=0;i<recent_projects.size();i++) {
        if (url == recent_projects.at(i)) {
            found = true;
            recent_projects.move(i, 0);
            break;
        }
    }
    if (!found) {
        recent_projects.insert(0, url);
        if (recent_projects.size() > MAXIMUM_RECENT_PROJECTS) {
            recent_projects.removeLast();
        }
    }
    save_recent_projects();
}

#define THROBBER_LIMIT 20
#define THROBBER_SIZE 50

MediaThrobber::MediaThrobber(QTreeWidgetItem* i) : pixmap(":/icons/throbber.png"), animation(0), item(i) {
    // set up throbber
    animation_update();
    animator.setInterval(20);
    connect(&animator, SIGNAL(timeout()), this, SLOT(animation_update()));
    animator.start();
}

void MediaThrobber::animation_update() {
    if (animation == THROBBER_LIMIT) {
        animation = 0;
    }
    item->setIcon(0, QIcon(pixmap.copy(THROBBER_SIZE*animation, 0, THROBBER_SIZE, THROBBER_SIZE)));
    animation++;
}

void MediaThrobber::stop(int icon_type) {
    animator.stop();
    switch (icon_type) {
    case ICON_TYPE_VIDEO: item->setIcon(0, QIcon(":/icons/videosource.png")); break;
    case ICON_TYPE_AUDIO: item->setIcon(0, QIcon(":/icons/audiosource.png")); break;
    case ICON_TYPE_ERROR: item->setIcon(0, QIcon::fromTheme("dialog-error")); break;
    }
    panel_project->source_table->viewport()->update();
    deleteLater();
}
