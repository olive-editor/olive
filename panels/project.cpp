#include "project.h"
#include "ui_project.h"
#include "io/media.h"

#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QCharRef>
#include <QMessageBox>

#include "panels/timeline.h"
#include "project/sequence.h"

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
	item->setData(0, Qt::UserRole + 1, QVariant::fromValue(reinterpret_cast<quintptr>(m)));

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
				item->setData(0, Qt::UserRole + 1, QVariant::fromValue(reinterpret_cast<quintptr>(m)));

				ui->treeWidget->addTopLevelItem(item);
			}
		}
		avformat_close_input(&pFormatCtx);
		delete [] filename;
	}
}
