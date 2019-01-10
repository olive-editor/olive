#include "exportdialog.h"

#include <QOpenGLWidget>
#include <QFileDialog>
#include <QThread>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QtMath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QProgressBar>

#include "debug.h"
#include "panels/panels.h"
#include "panels/viewer.h"
#include "panels/timeline.h"
#include "ui/viewerwidget.h"
#include "project/sequence.h"
#include "io/exportthread.h"
#include "playback/playback.h"
#include "mainwindow.h"

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
	QDialog(parent)
{
	setWindowTitle("Export \"" + sequence->name + "\"");
	setup_ui();

	rangeCombobox->setCurrentIndex(0);
	if (sequence->using_workarea) {
        rangeCombobox->setEnabled(true);
        if (sequence->enable_workarea) rangeCombobox->setCurrentIndex(1);
	}

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
		formatCombobox->addItem(format_strings[i]);
	}
	formatCombobox->setCurrentIndex(FORMAT_MPEG4);

	widthSpinbox->setValue(sequence->width);
	heightSpinbox->setValue(sequence->height);
	samplingRateSpinbox->setValue(sequence->audio_frequency);
	framerateSpinbox->setValue(sequence->frame_rate);
}

ExportDialog::~ExportDialog()
{}

void ExportDialog::format_changed(int index)
{
	format_vcodecs.clear();
	format_acodecs.clear();
	vcodecCombobox->clear();
	acodecCombobox->clear();

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
		dout << "[ERROR] Invalid format selection - this is a bug, please inform the developers";
	}

	AVCodec* codec_info;
	for (int i=0;i<format_vcodecs.size();i++) {
		codec_info = avcodec_find_encoder((enum AVCodecID) format_vcodecs.at(i));
		if (codec_info == NULL) {
			vcodecCombobox->addItem("NULL");
		} else {
			vcodecCombobox->addItem(codec_info->long_name);
		}
	}
	for (int i=0;i<format_acodecs.size();i++) {
		codec_info = avcodec_find_encoder((enum AVCodecID) format_acodecs.at(i));
		if (codec_info == NULL) {
			acodecCombobox->addItem("NULL");
		} else {
			acodecCombobox->addItem(codec_info->long_name);
		}
	}

	vcodecCombobox->setCurrentIndex(default_vcodec);
	acodecCombobox->setCurrentIndex(default_acodec);

	bool video_enabled = format_vcodecs.size() != 0;
	bool audio_enabled = format_acodecs.size() != 0;
	videoGroupbox->setChecked(video_enabled);
	audioGroupbox->setChecked(audio_enabled);
	videoGroupbox->setEnabled(video_enabled);
	audioGroupbox->setEnabled(audio_enabled);
}

void ExportDialog::render_thread_finished() {
	if (progressBar->value() < 100 && !cancelled) {
		QMessageBox::critical(this, "Export Failed", "Export failed - " + export_error, QMessageBox::Ok);
	}
	prep_ui_for_render(false);
	panel_sequence_viewer->viewer_widget->makeCurrent();
	panel_sequence_viewer->viewer_widget->initializeGL();
	update_ui(false);
	if (progressBar->value() == 100) accept();
}

void ExportDialog::prep_ui_for_render(bool r) {
	export_button->setEnabled(!r);
	cancel_button->setEnabled(!r);
	renderCancel->setEnabled(r);
}

void ExportDialog::export_action() {
	if (widthSpinbox->value()%2 == 1 || heightSpinbox->value()%2 == 1) {
		QMessageBox::critical(this, "Invalid dimensions", "Export width and height must both be even numbers/divisible by 2.", QMessageBox::Ok);
		return;
	}

	QString ext;
	switch (formatCombobox->currentIndex()) {
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
		switch (format_vcodecs.at(vcodecCombobox->currentIndex())) {
		case AV_CODEC_ID_BMP:
			ext = "bmp";
			break;
		case AV_CODEC_ID_MJPEG:
			ext = "jpg";
			break;
		case AV_CODEC_ID_JPEG2000:
			ext = "jp2";
			break;
		case AV_CODEC_ID_PSD:
			ext = "psd";
			break;
		case AV_CODEC_ID_PNG:
			ext = "png";
			break;
		case AV_CODEC_ID_TIFF:
			ext = "tif";
			break;
		default:
			dout << "[ERROR] Invalid codec selection for an image sequence";
			QMessageBox::critical(this, "Invalid codec", "Couldn't determine output parameters for the selected codec. This is a bug, please contact the developers.", QMessageBox::Ok);
			return;
		}
		break;
	case FORMAT_MP3:
		ext = "mp3";
		break;
	case FORMAT_MPEG1:
		if (videoGroupbox->isChecked() && !audioGroupbox->isChecked()) {
			ext = "m1v";
		} else if (!videoGroupbox->isChecked() && audioGroupbox->isChecked()) {
			ext = "m1a";
		} else {
			ext = "mpg";
		}
		break;
	case FORMAT_MPEG2:
		if (videoGroupbox->isChecked() && !audioGroupbox->isChecked()) {
			ext = "m2v";
		} else if (!videoGroupbox->isChecked() && audioGroupbox->isChecked()) {
			ext = "m2a";
		} else {
			ext = "mpg";
		}
		break;
	case FORMAT_MPEG4:
		if (videoGroupbox->isChecked() && !audioGroupbox->isChecked()) {
			ext = "m4v";
		} else if (!videoGroupbox->isChecked() && audioGroupbox->isChecked()) {
			ext = "m4a";
		} else {
			ext = "mp4";
		}
		break;
	case FORMAT_MPEGTS:
		ext = "ts";
		break;
	case FORMAT_MKV:
		if (!videoGroupbox->isChecked()) {
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
	case FORMAT_WAV:
		ext = "wav";
		break;
	case FORMAT_WEBM:
		ext = "webm";
		break;
	case FORMAT_WMV:
		if (videoGroupbox->isChecked()) {
			ext = "wmv";
		} else {
			ext = "wma";
		}
		break;
	default:
		dout << "[ERROR] Invalid format - this is a bug, please inform the developers";
		QMessageBox::critical(this, "Invalid format", "Couldn't determine output format. This is a bug, please contact the developers.", QMessageBox::Ok);
		return;
	}
	QString filename = QFileDialog::getSaveFileName(this, "Export Media", "", format_strings[formatCombobox->currentIndex()] + " (*." + ext + ")");
	if (!filename.isEmpty()) {
		if (!filename.endsWith("." + ext, Qt::CaseInsensitive)) {
			filename += "." + ext;
		}

		if (formatCombobox->currentIndex() == FORMAT_IMG) {
			int ext_location = filename.lastIndexOf('.');
			if (ext_location > filename.lastIndexOf('/')) {
				filename.insert(ext_location, 'd');
				filename.insert(ext_location, '5');
				filename.insert(ext_location, '0');
				filename.insert(ext_location, '%');
			}
		}

		et = new ExportThread();

		connect(et, SIGNAL(finished()), et, SLOT(deleteLater()));
		connect(et, SIGNAL(finished()), this, SLOT(render_thread_finished()));
		connect(et, SIGNAL(progress_changed(int, qint64)), this, SLOT(update_progress_bar(int, qint64)));

		closeActiveClips(sequence);

		mainWindow->autorecover_interval();

		rendering = true;
		panel_sequence_viewer->viewer_widget->context()->doneCurrent();
		panel_sequence_viewer->viewer_widget->context()->moveToThread(et);

		prep_ui_for_render(true);

		et->filename = filename;
		et->video_enabled = videoGroupbox->isChecked();
		if (et->video_enabled) {
			et->video_codec = format_vcodecs.at(vcodecCombobox->currentIndex());
			et->video_width = widthSpinbox->value();
			et->video_height = heightSpinbox->value();
			et->video_frame_rate = framerateSpinbox->value();
			et->video_compression_type = compressionTypeCombobox->currentData().toInt();
			et->video_bitrate = videobitrateSpinbox->value();
		}
		et->audio_enabled = audioGroupbox->isChecked();
		if (et->audio_enabled) {
			et->audio_codec = format_acodecs.at(acodecCombobox->currentIndex());
			et->audio_sampling_rate = samplingRateSpinbox->value();
			et->audio_bitrate = audiobitrateSpinbox->value();
		}

		et->start_frame = 0;
		et->end_frame = sequence->getEndFrame(); // entire sequence
		if (rangeCombobox->currentIndex() == 1) {
			et->start_frame = qMax(sequence->workarea_in, et->start_frame);
			et->end_frame = qMin(sequence->workarea_out, et->end_frame);
		}

		et->ed = this;
		cancelled = false;

		et->start();
	}
}

void ExportDialog::update_progress_bar(int value, qint64 remaining_ms) {
	// convert ms to H:MM:SS
	int seconds = qFloor(remaining_ms*0.001)%60;
	int minutes = qFloor(remaining_ms/60000)%60;
	int hours = qFloor(remaining_ms/3600000);
	progressBar->setFormat("%p% (ETA: " + QString::number(hours) + ":" + QString::number(minutes).rightJustified(2, '0') + ":" + QString::number(seconds).rightJustified(2, '0') + ")");

	progressBar->setValue(value);
}

void ExportDialog::cancel_render() {
	panel_sequence_viewer->viewer_widget->force_quit = true;
	et->continueEncode = false;
	cancelled = true;
}

void ExportDialog::vcodec_changed(int index) {
	compressionTypeCombobox->clear();
	if ((format_vcodecs.size() > 0 && format_vcodecs.at(index) == AV_CODEC_ID_H264)) {
		compressionTypeCombobox->setEnabled(true);
		compressionTypeCombobox->addItem("Quality-based (Constant Rate Factor)", COMPRESSION_TYPE_CFR);
//		compressionTypeCombobox->addItem("File size-based (Two-Pass)", COMPRESSION_TYPE_TARGETSIZE);
//		compressionTypeCombobox->addItem("Average bitrate (Two-Pass)", COMPRESSION_TYPE_TARGETBR);
	} else {
		compressionTypeCombobox->addItem("Constant Bitrate", COMPRESSION_TYPE_CBR);
		compressionTypeCombobox->setCurrentIndex(0);
		compressionTypeCombobox->setEnabled(false);
	}
}

void ExportDialog::comp_type_changed(int) {
	videobitrateSpinbox->setToolTip("");
	videobitrateSpinbox->setMinimum(0);
	videobitrateSpinbox->setMaximum(99.99);
	switch (compressionTypeCombobox->currentData().toInt()) {
	case COMPRESSION_TYPE_CBR:
	case COMPRESSION_TYPE_TARGETBR:
		videoBitrateLabel->setText("Bitrate (Mbps):");
		videobitrateSpinbox->setValue(qMax(0.5, (double) qRound((0.01528 * sequence->height) - 4.5)));
		break;
	case COMPRESSION_TYPE_CFR:
		videoBitrateLabel->setText("Quality (CRF):");
		videobitrateSpinbox->setValue(36);
		videobitrateSpinbox->setMaximum(51);
		videobitrateSpinbox->setToolTip("Quality Factor:\n\n0 = lossless\n17-18 = visually lossless (compressed, but unnoticeable)\n23 = high quality\n51 = lowest quality possible");
		break;
	case COMPRESSION_TYPE_TARGETSIZE:
		videoBitrateLabel->setText("Target File Size (MB):");
		videobitrateSpinbox->setValue(100);
		break;
	}
}

void ExportDialog::setup_ui() {
	QVBoxLayout* verticalLayout = new QVBoxLayout(this);

	QHBoxLayout* horizontalLayout = new QHBoxLayout();

	horizontalLayout->addWidget(new QLabel("Format:"));

	formatCombobox = new QComboBox(this);

	horizontalLayout->addWidget(formatCombobox);

	verticalLayout->addLayout(horizontalLayout);

	QHBoxLayout* horizontalLayout_4 = new QHBoxLayout();

	horizontalLayout_4->addWidget(new QLabel("Range:"));

	rangeCombobox = new QComboBox(this);
	rangeCombobox->addItem("Entire Sequence");
	rangeCombobox->addItem("In to Out");

	horizontalLayout_4->addWidget(rangeCombobox);

	verticalLayout->addLayout(horizontalLayout_4);

	videoGroupbox = new QGroupBox(this);
	videoGroupbox->setTitle("Video");
	videoGroupbox->setFlat(false);
	videoGroupbox->setCheckable(true);

	QGridLayout* videoGridLayout = new QGridLayout(videoGroupbox);

	videoGridLayout->addWidget(new QLabel("Codec:"), 0, 0, 1, 1);
	vcodecCombobox = new QComboBox(videoGroupbox);
	videoGridLayout->addWidget(vcodecCombobox, 0, 1, 1, 1);

	videoGridLayout->addWidget(new QLabel("Width:"), 1, 0, 1, 1);
	widthSpinbox = new QSpinBox(videoGroupbox);
	widthSpinbox->setMaximum(16777216);
	videoGridLayout->addWidget(widthSpinbox, 1, 1, 1, 1);

	videoGridLayout->addWidget(new QLabel("Height:"), 2, 0, 1, 1);
	heightSpinbox = new QSpinBox(videoGroupbox);
	heightSpinbox->setMaximum(16777216);
	videoGridLayout->addWidget(heightSpinbox, 2, 1, 1, 1);

	videoGridLayout->addWidget(new QLabel("Frame Rate:"), 3, 0, 1, 1);
	framerateSpinbox = new QDoubleSpinBox(videoGroupbox);
	framerateSpinbox->setMaximum(60);
	framerateSpinbox->setValue(0);
	videoGridLayout->addWidget(framerateSpinbox, 3, 1, 1, 1);

	videoGridLayout->addWidget(new QLabel("Compression Type:"), 4, 0, 1, 1);
	compressionTypeCombobox = new QComboBox(videoGroupbox);
	compressionTypeCombobox->addItem("Quality-based (Constant Rate Factor)");
	compressionTypeCombobox->addItem("File size-based (Two-Pass)");
	videoGridLayout->addWidget(compressionTypeCombobox, 4, 1, 1, 1);

	videoBitrateLabel = new QLabel(videoGroupbox);
	videoGridLayout->addWidget(videoBitrateLabel, 5, 0, 1, 1);
	videobitrateSpinbox = new QDoubleSpinBox(videoGroupbox);
	videobitrateSpinbox->setMaximum(100);
	videobitrateSpinbox->setValue(2);
	videoGridLayout->addWidget(videobitrateSpinbox, 5, 1, 1, 1);

	verticalLayout->addWidget(videoGroupbox);

	audioGroupbox = new QGroupBox(this);
	audioGroupbox->setTitle("Audio");
	audioGroupbox->setCheckable(true);

	QGridLayout* audioGridLayout = new QGridLayout(audioGroupbox);

	audioGridLayout->addWidget(new QLabel("Codec:"), 0, 0, 1, 1);
	acodecCombobox = new QComboBox(audioGroupbox);
	audioGridLayout->addWidget(acodecCombobox, 0, 1, 1, 1);

	audioGridLayout->addWidget(new QLabel("Sampling Rate:"), 1, 0, 1, 1);
	samplingRateSpinbox = new QSpinBox(audioGroupbox);
	samplingRateSpinbox->setMaximum(96000);
	samplingRateSpinbox->setValue(0);
	audioGridLayout->addWidget(samplingRateSpinbox, 1, 1, 1, 1);

	audioGridLayout->addWidget(new QLabel("Bitrate (Kbps/CBR):"), 3, 0, 1, 1);
	audiobitrateSpinbox = new QSpinBox(audioGroupbox);
	audiobitrateSpinbox->setMaximum(320);
	audiobitrateSpinbox->setValue(256);
	audioGridLayout->addWidget(audiobitrateSpinbox, 3, 1, 1, 1);

	verticalLayout->addWidget(audioGroupbox);

	QHBoxLayout* progressLayout = new QHBoxLayout();
	progressBar = new QProgressBar(this);
	progressBar->setFormat("%p% (ETA: 0:00:00)");
	progressBar->setEnabled(false);
	progressBar->setValue(0);
	progressLayout->addWidget(progressBar);

	renderCancel = new QPushButton(this);
	renderCancel->setText("x");
	renderCancel->setEnabled(false);
	renderCancel->setMaximumSize(QSize(20, 16777215));
	connect(renderCancel, SIGNAL(clicked(bool)), this, SLOT(cancel_render()));
	progressLayout->addWidget(renderCancel);

	verticalLayout->addLayout(progressLayout);

	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->addStretch();

	export_button = new QPushButton(this);
	export_button->setText("Export");
	connect(export_button, SIGNAL(clicked(bool)), this, SLOT(export_action()));

	buttonLayout->addWidget(export_button);

	cancel_button = new QPushButton(this);
	cancel_button->setText("Cancel");
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));

	buttonLayout->addWidget(cancel_button);

	buttonLayout->addStretch();

	verticalLayout->addLayout(buttonLayout);

	connect(formatCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(format_changed(int)));
	connect(compressionTypeCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(comp_type_changed(int)));
	connect(vcodecCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(vcodec_changed(int)));
}
