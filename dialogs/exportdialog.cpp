#include "exportdialog.h"
#include "ui_exportdialog.h"

#include <QOpenGLWidget>
#include <QFileDialog>
#include <QThread>
#include <QDebug>
#include <QMessageBox>

#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "ui/viewerwidget.h"
#include "project/sequence.h"
#include "io/exportthread.h"

extern "C" {
	#include <libavformat/avformat.h>
}

enum ExportFormats {
	FORMAT_3GPP,
	FORMAT_AIFF,
	FORMAT_APNG,
	FORMAT_AVI,
	FORMAT_DNXHD,
	FORMAT_AC3,
	FORMAT_FLV,
	FORMAT_GIF,
	FORMAT_IMG,
	FORMAT_MP2,
	FORMAT_MP3,
	FORMAT_MPEG1,
	FORMAT_MPEG2,
	FORMAT_MPEG4,
	FORMAT_MPEGTS,
	FORMAT_MKV,
	FORMAT_OGG,
	FORMAT_MOV,
	FORMAT_WAV,
	FORMAT_WEBM,
	FORMAT_WMV,
	FORMAT_SIZE
};

ExportDialog::ExportDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ExportDialog)
{
	ui->setupUi(this);

	format_strings.resize(FORMAT_SIZE);
	format_strings[FORMAT_3GPP] = "3GPP";
	format_strings[FORMAT_AIFF] = "AIFF";
	format_strings[FORMAT_APNG] = "Animated PNG";
	format_strings[FORMAT_AVI] = "AVI";
	format_strings[FORMAT_DNXHD] = "DNxHD";
	format_strings[FORMAT_AC3] = "Dolby Digital (AC3)";
	format_strings[FORMAT_FLV] = "FLV";
	format_strings[FORMAT_GIF] = "GIF";
	format_strings[FORMAT_IMG] = "Image Sequence";
	format_strings[FORMAT_MP2] = "MP2 Audio";
	format_strings[FORMAT_MP3] = "MP3 Audio";
	format_strings[FORMAT_MPEG1] = "MPEG-1 Video";
	format_strings[FORMAT_MPEG2] = "MPEG-2 Video";
	format_strings[FORMAT_MPEG4] = "MPEG-4 Video";
	format_strings[FORMAT_MPEGTS] = "MPEG-TS";
	format_strings[FORMAT_MKV] = "Matroska MKV";
	format_strings[FORMAT_OGG] = "Ogg";
	format_strings[FORMAT_MOV] = "QuickTime MOV";
	format_strings[FORMAT_WAV] = "WAVE Audio";
	format_strings[FORMAT_WEBM] = "WebM";
	format_strings[FORMAT_WMV] = "Windows Media";

	for (int i=0;i<FORMAT_SIZE;i++) {
		ui->formatCombobox->addItem(format_strings[i]);
	}
	ui->formatCombobox->setCurrentIndex(FORMAT_MPEG4);

    ui->widthSpinbox->setValue(sequence->width);
    ui->heightSpinbox->setValue(sequence->height);
    ui->samplingRateSpinbox->setValue(sequence->audio_frequency);
    ui->framerateSpinbox->setValue(sequence->frame_rate);
}

ExportDialog::~ExportDialog()
{
	delete ui;
}

void ExportDialog::on_formatCombobox_currentIndexChanged(int index)
{
	format_vcodecs.clear();
	format_acodecs.clear();
	ui->vcodecCombobox->clear();
	ui->acodecCombobox->clear();

	int default_vcodec = 0;
	int default_acodec = 0;

	switch (index) {
	case FORMAT_3GPP:
		format_vcodecs.append(AV_CODEC_ID_MPEG4);
		format_vcodecs.append(AV_CODEC_ID_H264);

		format_acodecs.append(AV_CODEC_ID_AAC);

		default_vcodec = 1;
		break;
	case FORMAT_AIFF:
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);
		break;
	case FORMAT_APNG:
		format_vcodecs.append(AV_CODEC_ID_APNG);
		break;
	case FORMAT_AVI:
		format_vcodecs.append(AV_CODEC_ID_H264);
		format_vcodecs.append(AV_CODEC_ID_MPEG4);
		format_vcodecs.append(AV_CODEC_ID_MJPEG);
		format_vcodecs.append(AV_CODEC_ID_MSVIDEO1);
		format_vcodecs.append(AV_CODEC_ID_RAWVIDEO);
		format_vcodecs.append(AV_CODEC_ID_HUFFYUV);
		format_vcodecs.append(AV_CODEC_ID_DVVIDEO);

		format_acodecs.append(AV_CODEC_ID_AAC);
		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_FLAC);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);

		default_vcodec = 3;
		default_acodec = 5;
		break;
	case FORMAT_DNXHD:
		format_vcodecs.append(AV_CODEC_ID_DNXHD);

		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);
		break;
	case FORMAT_AC3:
		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_EAC3);
		break;
	case FORMAT_FLV:
		format_vcodecs.append(AV_CODEC_ID_FLV1);

		format_acodecs.append(AV_CODEC_ID_MP3);
		break;
	case FORMAT_GIF:
		format_vcodecs.append(AV_CODEC_ID_GIF);
		break;
	case FORMAT_IMG:
		format_vcodecs.append(AV_CODEC_ID_BMP);
		format_vcodecs.append(AV_CODEC_ID_MJPEG);
		format_vcodecs.append(AV_CODEC_ID_JPEG2000);
		format_vcodecs.append(AV_CODEC_ID_PSD);
		format_vcodecs.append(AV_CODEC_ID_PNG);
		format_vcodecs.append(AV_CODEC_ID_TIFF);

		default_vcodec = 4;
		break;
	case FORMAT_MP2:
		format_acodecs.append(AV_CODEC_ID_MP2);
		break;
	case FORMAT_MP3:
		format_acodecs.append(AV_CODEC_ID_MP3);
		break;
	case FORMAT_MPEG1:
		format_vcodecs.append(AV_CODEC_ID_MPEG1VIDEO);

		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);

		default_acodec = 1;
		break;
	case FORMAT_MPEG2:
		format_vcodecs.append(AV_CODEC_ID_MPEG2VIDEO);

		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);

		default_acodec = 1;
		break;
	case FORMAT_MPEG4:
		format_vcodecs.append(AV_CODEC_ID_MPEG4);
		format_vcodecs.append(AV_CODEC_ID_H264);

		format_acodecs.append(AV_CODEC_ID_AAC);
		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);

		default_vcodec = 1;
		break;
	case FORMAT_MPEGTS:
		format_vcodecs.append(AV_CODEC_ID_MPEG2VIDEO);

		format_acodecs.append(AV_CODEC_ID_AAC);
		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);

		default_acodec = 2;
		break;
	case FORMAT_MKV:
		format_vcodecs.append(AV_CODEC_ID_MPEG4);
		format_vcodecs.append(AV_CODEC_ID_H264);

		format_acodecs.append(AV_CODEC_ID_AAC);
		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_EAC3);
		format_acodecs.append(AV_CODEC_ID_FLAC);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);
		format_acodecs.append(AV_CODEC_ID_OPUS);
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);
		format_acodecs.append(AV_CODEC_ID_VORBIS);
		format_acodecs.append(AV_CODEC_ID_WAVPACK);
		format_acodecs.append(AV_CODEC_ID_WMAV1);
		format_acodecs.append(AV_CODEC_ID_WMAV2);

		default_vcodec = 1;
		break;
	case FORMAT_OGG:
		format_vcodecs.append(AV_CODEC_ID_THEORA);

		format_acodecs.append(AV_CODEC_ID_OPUS);
		format_acodecs.append(AV_CODEC_ID_VORBIS);

        default_acodec = 1;
		break;
	case FORMAT_MOV:
		format_vcodecs.append(AV_CODEC_ID_QTRLE);
		format_vcodecs.append(AV_CODEC_ID_MPEG4);
		format_vcodecs.append(AV_CODEC_ID_H264);
		format_vcodecs.append(AV_CODEC_ID_MJPEG);
		format_vcodecs.append(AV_CODEC_ID_PRORES);

		format_acodecs.append(AV_CODEC_ID_AAC);
		format_acodecs.append(AV_CODEC_ID_AC3);
		format_acodecs.append(AV_CODEC_ID_MP2);
		format_acodecs.append(AV_CODEC_ID_MP3);
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);

		default_vcodec = 2;
		break;
	case FORMAT_WAV:
		format_acodecs.append(AV_CODEC_ID_PCM_S16LE);
		break;
	case FORMAT_WEBM:
		format_vcodecs.append(AV_CODEC_ID_VP8);
		format_vcodecs.append(AV_CODEC_ID_VP9);

		format_acodecs.append(AV_CODEC_ID_OPUS);
		format_acodecs.append(AV_CODEC_ID_VORBIS);

		default_vcodec = 1;
		break;
	case FORMAT_WMV:
		format_vcodecs.append(AV_CODEC_ID_WMV1);
		format_vcodecs.append(AV_CODEC_ID_WMV2);

		format_acodecs.append(AV_CODEC_ID_WMAV1);
		format_acodecs.append(AV_CODEC_ID_WMAV2);

		default_vcodec = 1;
		default_acodec = 1;
		break;
	default:
		qDebug() << "[ERROR] Invalid format selection - this is a bug, please inform the developers";
	}

	AVCodec* codec_info;
	for (int i=0;i<format_vcodecs.size();i++) {
		codec_info = avcodec_find_encoder((enum AVCodecID) format_vcodecs.at(i));
		if (codec_info == NULL) {
			ui->vcodecCombobox->addItem("NULL");
		} else {
			ui->vcodecCombobox->addItem(codec_info->long_name);
		}
	}
	for (int i=0;i<format_acodecs.size();i++) {
		codec_info = avcodec_find_encoder((enum AVCodecID) format_acodecs.at(i));
		if (codec_info == NULL) {
			ui->acodecCombobox->addItem("NULL");
		} else {
			ui->acodecCombobox->addItem(codec_info->long_name);
		}
	}

	ui->vcodecCombobox->setCurrentIndex(default_vcodec);
	ui->acodecCombobox->setCurrentIndex(default_acodec);

	bool video_enabled = format_vcodecs.size() != 0;
	bool audio_enabled = format_acodecs.size() != 0;
	ui->videoGroupbox->setChecked(video_enabled);
	ui->audioGroupbox->setChecked(audio_enabled);
	ui->videoGroupbox->setEnabled(video_enabled);
	ui->audioGroupbox->setEnabled(audio_enabled);
}

void ExportDialog::on_pushButton_2_clicked()
{
	close();
}

void ExportDialog::render_thread_finished() {
    if (ui->progressBar->value() < 100 && !cancelled) {
        QMessageBox::critical(this, "Export Failed", "Export failed - " + export_error, QMessageBox::Ok);
    }
    prep_ui_for_render(false);
    panel_viewer->viewer_widget->makeCurrent();
    panel_viewer->viewer_widget->initializeGL();
}

void ExportDialog::prep_ui_for_render(bool rendering) {
    ui->pushButton->setEnabled(!rendering);
    ui->pushButton_2->setEnabled(!rendering);
    ui->renderCancel->setEnabled(rendering);
}

void ExportDialog::on_pushButton_clicked()
{
	QString ext;
	switch (ui->formatCombobox->currentIndex()) {
	case FORMAT_3GPP:
		ext = "3gp";
		break;
	case FORMAT_AIFF:
		ext = "aiff";
		break;
	case FORMAT_APNG:
		ext = "apng";
		break;
	case FORMAT_AVI:
		ext = "avi";
		break;
	case FORMAT_DNXHD:
		ext = "mxf";
		break;
	case FORMAT_AC3:
		ext = "ac3";
		break;
	case FORMAT_FLV:
		ext = "flv";
		break;
	case FORMAT_GIF:
		ext = "gif";
		break;
	case FORMAT_IMG:
		// ext_name = "";
		// ext = "";
		// ext is determined from codec, but we prevent the switch from going to 'default' here
		qDebug() << "[INFO] Image export not implemented yet";
        QMessageBox::information(this, "Image sequence support coming soon", "Sorry, image sequence exporting is currently unsupported. Check back soon (or hound GitHub issue #83).", QMessageBox::Ok);
		return;
		break;
	case FORMAT_MP3:
		ext = "mp3";
		break;
	case FORMAT_MPEG1:
		if (ui->videoGroupbox->isChecked() && !ui->audioGroupbox->isChecked()) {
			ext = "m1v";
		} else if (!ui->videoGroupbox->isChecked() && ui->audioGroupbox->isChecked()) {
			ext = "m1a";
		} else {
			ext = "mpg";
		}
		break;
	case FORMAT_MPEG2:
		if (ui->videoGroupbox->isChecked() && !ui->audioGroupbox->isChecked()) {
			ext = "m2v";
		} else if (!ui->videoGroupbox->isChecked() && ui->audioGroupbox->isChecked()) {
			ext = "m2a";
		} else {
			ext = "mpg";
		}
		break;
	case FORMAT_MPEG4:
		if (ui->videoGroupbox->isChecked() && !ui->audioGroupbox->isChecked()) {
			ext = "m4v";
		} else if (!ui->videoGroupbox->isChecked() && ui->audioGroupbox->isChecked()) {
			ext = "m4a";
		} else {
			ext = "mp4";
		}
		break;
	case FORMAT_MPEGTS:
		ext = "ts";
		break;
	case FORMAT_MKV:
		if (!ui->videoGroupbox->isChecked()) {
			ext = "mka";
		} else {
			ext = "mkv";
		}
		break;
	case FORMAT_OGG:
		ext = "ogg";
		break;
	case FORMAT_MOV:
		ext = "mov";
		break;
	case FORMAT_WEBM:
		ext = "webm";
		break;
	case FORMAT_WMV:
		if (ui->videoGroupbox->isChecked()) {
			ext = "wmv";
		} else {
			ext = "wma";
		}
		break;
	default:
		qDebug() << "[ERROR] Invalid format - this is a bug, please inform the developers";
		return;
	}
	QString filename = QFileDialog::getSaveFileName(this, "Export Media", "", format_strings[ui->formatCombobox->currentIndex()] + " (*." + ext + ")");
	if (!filename.isEmpty()) {
        if (!filename.endsWith("." + ext, Qt::CaseInsensitive)) {
            filename += "." + ext;
        }
        et = new ExportThread();

		et->surface.create();

		connect(et, SIGNAL(finished()), et, SLOT(deleteLater()));
        connect(et, SIGNAL(finished()), this, SLOT(render_thread_finished()));
		connect(et, SIGNAL(progress_changed(int)), this, SLOT(update_progress_bar(int)));

        panel_viewer->viewer_widget->multithreaded = false;
        panel_viewer->viewer_widget->enable_paint = false;
		panel_viewer->viewer_widget->context()->doneCurrent();
		panel_viewer->viewer_widget->context()->moveToThread(et);
        panel_viewer->viewer_widget->force_audio = true;

        prep_ui_for_render(true);

		et->filename = filename;
		et->video_enabled = ui->videoGroupbox->isChecked();
		et->video_codec = format_vcodecs.at(ui->vcodecCombobox->currentIndex());
		et->video_width = ui->widthSpinbox->value();
		et->video_height = ui->heightSpinbox->value();
		et->video_frame_rate = ui->framerateSpinbox->value();
		et->video_bitrate = ui->videobitrateSpinbox->value();
		et->audio_enabled = ui->audioGroupbox->isChecked();
		et->audio_codec = format_acodecs.at(ui->acodecCombobox->currentIndex());
		et->audio_sampling_rate = 48000;
		et->audio_bitrate = ui->audiobitrateSpinbox->value();

        et->ed = this;
        cancelled = false;

		et->start();
	}
}

void ExportDialog::update_progress_bar(int value) {
	ui->progressBar->setValue(value);
}

void ExportDialog::on_renderCancel_clicked() {
    et->fail = true;
    cancelled = true;
}
