#include "project.h"
#include "ui_project.h"
#include "io/media.h"

#include "panels/panels.h"
#include "panels/timeline.h"
#include "panels/viewer.h"

#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QCharRef>
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "panels/timeline.h"
#include "project/sequence.h"
#include "project/effect.h"

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}

Project::Project(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::Project)
{
	ui->setupUi(this);
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

void Project::new_sequence(Sequence *s) {
	Media* m = new Media();
	m->is_sequence = true;
	m->sequence = s;

	QTreeWidgetItem* item = new QTreeWidgetItem();
	item->setText(0, s->name);
    set_media_of_tree(item, m);

	ui->treeWidget->addTopLevelItem(item);
	source_table = ui->treeWidget;
}

void Project::import_dialog() {
	QStringList files = QFileDialog::getOpenFileNames(this, "Import media...", "", "All Files (*.*)");
	for (int i=0;i<files.count();i++) {
		QString file(files.at(i));
		char* filename = NULL;

		// heuristic to determine whether file is part of an image sequence
		int lastcharindex = file.lastIndexOf(".")-1;
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

		QByteArray ba = file.toLatin1();
		filename = new char[ba.size()+1];
		strcpy(filename, ba.data());

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
				m->is_sequence = false;
				m->url = file;

				// detect video/audio streams in file
				for (int i=0;i<(int)pFormatCtx->nb_streams;i++) {
					// Find the decoder for the video stream
					if (avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id) == NULL) {
						qDebug() << "[ERROR] Unsupported codec in stream %d.\n";
					} else {
						if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
							qDebug() << "[WARNING] INFINITE_LENGTH calculation is inaccurate in this build\n";
							// TODO BETTER infinite length calculator
							bool infinite_length = (pFormatCtx->streams[i]->nb_frames == 0);
//							bool infinite_length = false;

							m->video_tracks.append({i, pFormatCtx->streams[i]->codecpar->width, pFormatCtx->streams[i]->codecpar->height, infinite_length});
						} else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
							m->audio_tracks.append({i, 0, 0, false});
						}
					}
				}
				m->name = files.at(i).mid(files.at(i).lastIndexOf('/')+1);
				m->length = pFormatCtx->duration;

				QTreeWidgetItem* item = new QTreeWidgetItem();
				if (m->video_tracks.size() == 0) {
					item->setIcon(0, QIcon(":/icons/audiosource.png"));
				} else {
					item->setIcon(0, QIcon(":/icons/videosource.png"));
				}
				item->setText(0, m->name);
				item->setText(1, QString::number(m->length));
                set_media_of_tree(item, m);

				ui->treeWidget->addTopLevelItem(item);
			}
		}
		avformat_close_input(&pFormatCtx);
		delete [] filename;
	}
}

Media* Project::get_media_from_tree(QTreeWidgetItem* item) {
    return reinterpret_cast<Media*>(item->data(0, Qt::UserRole + 1).value<quintptr>());
}

void Project::set_media_of_tree(QTreeWidgetItem* item, Media* media) {
    item->setData(0, Qt::UserRole + 1, QVariant::fromValue(reinterpret_cast<quintptr>(media)));
}

void Project::remove_item(int i) {
    Media* m = get_media_from_tree(ui->treeWidget->topLevelItem(i));
    if (m->is_sequence) {
        delete m->sequence;
    }
    delete m;
    ui->treeWidget->takeTopLevelItem(i);
}

void Project::clear() {
    int len = ui->treeWidget->topLevelItemCount();
    for (int i=0;i<len;i++) {
        remove_item(i);
    }
}

void Project::set_sequence(Sequence* s) {
    panel_timeline->set_sequence(s);
    panel_viewer->set_sequence(s);
}

void Project::load_project() {
    // clear existing project
    set_sequence(NULL);
    clear();

    QFile file("C:/Users/Matt/Desktop/test.xml");
    if (!file.open(QIODevice::ReadOnly/* | QIODevice::Text*/)) {
        qDebug() << "[ERROR] Could not open file";
        return;
    }

    QXmlStreamReader stream(&file);

    while (!stream.atEnd()) {
        stream.readNext();

    }
    if (stream.hasError()) {
        qDebug() << "[ERROR] Error parsing XML." << stream.error();
    }
}

void Project::save_project() {
    QFile file("C:/Users/Matt/Desktop/test.xml");
    if (!file.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
        qDebug() << "[ERROR] Could not open file";
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    int len = ui->treeWidget->topLevelItemCount();
    stream.writeStartElement("media");
    for (int i=0;i<len;i++) {
        Media* m = get_media_from_tree(ui->treeWidget->topLevelItem(i));
        if (!m->is_sequence) {
            m->save_id = i;
            stream.writeStartElement("footage");
            stream.writeAttribute("id", QString::number(m->save_id));
            stream.writeTextElement("name", m->name);
            stream.writeTextElement("url", m->url);
            stream.writeEndElement();
        }
    }
    stream.writeEndElement();

    stream.writeStartElement("timeline");
    for (int i=0;i<len;i++) {
        Media* m = get_media_from_tree(ui->treeWidget->topLevelItem(i));
        if (m->is_sequence) {
            Sequence* s = m->sequence;
            stream.writeStartElement("sequence");
            stream.writeTextElement("name", s->name);
            stream.writeTextElement("width", QString::number(s->width));
            stream.writeTextElement("height", QString::number(s->height));
            stream.writeTextElement("framerate", QString::number(s->frame_rate));
            stream.writeTextElement("afreq", QString::number(s->audio_frequency));
            stream.writeTextElement("alayout", QString::number(s->audio_layout));
            stream.writeStartElement("clips");
            for (int i=0;i<s->clip_count();i++) {
                Clip* c = s->get_clip(i);

                stream.writeStartElement("clip");
                stream.writeTextElement("name", c->name);
                stream.writeTextElement("clipin", QString::number(c->clip_in));
                stream.writeTextElement("in", QString::number(c->timeline_in));
                stream.writeTextElement("out", QString::number(c->timeline_out));
                stream.writeTextElement("track", QString::number(c->track));
                stream.writeStartElement("color");
                stream.writeAttribute("r", QString::number(c->color_r));
                stream.writeAttribute("g", QString::number(c->color_g));
                stream.writeAttribute("b", QString::number(c->color_b));
                stream.writeEndElement();
                stream.writeTextElement("media", QString::number(c->media->save_id));
                stream.writeTextElement("stream", QString::number(c->media_stream->file_index));
                stream.writeStartElement("effects");
                for (int j=0;j<c->effects.size();j++) {
                    stream.writeStartElement("effect");
                    Effect* e = c->effects.at(j);
                    stream.writeAttribute("id", QString::number(e->id));
                    e->save(&stream);
                    stream.writeEndElement();
                }
                stream.writeEndElement();
                stream.writeEndElement();
            }
            stream.writeEndElement();
        }
    }
    stream.writeEndElement();

    stream.writeEndDocument();

    file.close();
}
