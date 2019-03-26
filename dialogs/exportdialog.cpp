/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "exportdialog.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <QOpenGLWidget>
#include <QFileDialog>
#include <QThread>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QtMath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "global/global.h"
#include "dialogs/advancedvideodialog.h"
#include "panels/panels.h"
#include "ui/viewerwidget.h"
#include "rendering/renderfunctions.h"
#include "rendering/audio.h"
#include "rendering/exportthread.h"
#include "ui/mainwindow.h"

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
  setWindowTitle(tr("Export \"%1\"").arg(olive::ActiveSequence->name));
  setup_ui();

  rangeCombobox->setCurrentIndex(0);
  if (olive::ActiveSequence->using_workarea) {
    rangeCombobox->setEnabled(true);
    rangeCombobox->setCurrentIndex(1);
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

  // default to sequence's native dimensions
  widthSpinbox->setValue(olive::ActiveSequence->width);
  heightSpinbox->setValue(olive::ActiveSequence->height);
  samplingRateSpinbox->setValue(olive::ActiveSequence->audio_frequency);
  framerateSpinbox->setValue(olive::ActiveSequence->frame_rate);

  // set some advanced defaults
  vcodec_params.threads = 0;
}

void ExportDialog::add_codec_to_combobox(QComboBox* box, enum AVCodecID codec) {
  QString codec_name;

  AVCodec* codec_info = avcodec_find_encoder(codec);

  if (codec_info == nullptr) {
    codec_name = tr("Unknown codec name %1").arg(static_cast<int>(codec));
  } else {
    codec_name = codec_info->long_name;
  }

  box->addItem(codec_name, codec);
}

void ExportDialog::format_changed(int index) {
  vcodecCombobox->clear();
  acodecCombobox->clear();

  int default_vcodec = 0;
  int default_acodec = 0;

  switch (index) {
  case FORMAT_3GPP:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG4);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H264);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H265);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AAC);

    default_vcodec = 1;
    break;
  case FORMAT_AIFF:
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);
    break;
  case FORMAT_APNG:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_APNG);
    break;
  case FORMAT_AVI:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H264);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG4);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MJPEG);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MSVIDEO1);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_RAWVIDEO);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_HUFFYUV);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_DVVIDEO);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_FLAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);

    default_vcodec = 3;
    default_acodec = 5;
    break;
  case FORMAT_DNXHD:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_DNXHD);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);
    break;
  case FORMAT_AC3:
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_EAC3);
    break;
  case FORMAT_FLV:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_FLV1);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    break;
  case FORMAT_GIF:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_GIF);
    break;
  case FORMAT_IMG:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_BMP);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MJPEG);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_JPEG2000);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_PSD);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_PNG);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_TIFF);

    default_vcodec = 4;
    break;
  case FORMAT_MP2:
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    break;
  case FORMAT_MP3:
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    break;
  case FORMAT_MPEG1:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG1VIDEO);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);

    default_acodec = 1;
    break;
  case FORMAT_MPEG2:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG2VIDEO);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);

    default_acodec = 1;
    break;
  case FORMAT_MPEG4:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG4);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H264);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H265);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);

    default_vcodec = 1;
    break;
  case FORMAT_MPEGTS:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG2VIDEO);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);

    default_acodec = 2;
    break;
  case FORMAT_MKV:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG4);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H264);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H265);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_EAC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_FLAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_OPUS);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_VORBIS);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_WAVPACK);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_WMAV1);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_WMAV2);

    default_vcodec = 1;
    break;
  case FORMAT_OGG:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_THEORA);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_OPUS);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_VORBIS);

    default_acodec = 1;
    break;
  case FORMAT_MOV:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_QTRLE);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MPEG4);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H264);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_H265);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_MJPEG);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_PRORES);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AAC);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_AC3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP2);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_MP3);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);

    default_vcodec = 2;
    break;
  case FORMAT_WAV:
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_PCM_S16LE);
    break;
  case FORMAT_WEBM:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_VP8);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_VP9);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_OPUS);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_VORBIS);

    default_vcodec = 1;
    break;
  case FORMAT_WMV:
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_WMV1);
    add_codec_to_combobox(vcodecCombobox, AV_CODEC_ID_WMV2);

    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_WMAV1);
    add_codec_to_combobox(acodecCombobox, AV_CODEC_ID_WMAV2);

    default_vcodec = 1;
    default_acodec = 1;
    break;
  default:
    qCritical() << "Invalid format selection - this is a bug, please inform the developers";
  }

  vcodecCombobox->setCurrentIndex(default_vcodec);
  acodecCombobox->setCurrentIndex(default_acodec);

  bool video_enabled = vcodecCombobox->count() != 0;
  bool audio_enabled = acodecCombobox->count() != 0;
  videoGroupbox->setChecked(video_enabled);
  audioGroupbox->setChecked(audio_enabled);
  videoGroupbox->setEnabled(video_enabled);
  audioGroupbox->setEnabled(audio_enabled);
}

void ExportDialog::export_thread_finished() {
  // Determine if the export succeeded
  bool succeeded = (progressBar->value() == 100);

  // If it failed and we didn't cancel it, it must have errored out. Show an error message.
  if (!succeeded && !export_thread_->WasInterrupted()) {
    QMessageBox::critical(
          this,
          tr("Export Failed"),
          tr("Export failed - %1").arg(export_thread_->GetError()),
          QMessageBox::Ok
          );
  }

  // Clear audio buffer
  clear_audio_ibuffer();

  // Re-enable/disable UI widgets based on the rendering state
  prep_ui_for_render(false);

  // Move OpenGL context back to the sequence viewer
  panel_sequence_viewer->viewer_widget()->makeCurrent();
  panel_sequence_viewer->viewer_widget()->initializeGL();

  // Update the application UI
  update_ui(false);

  // Disconnect cancel button from export thread
  disconnect(renderCancel, SIGNAL(clicked(bool)), export_thread_, SLOT(Interrupt()));

  // Free the export thread
  export_thread_->deleteLater();

  // If the export succeeded, close the dialog
  if (succeeded) {
    accept();
  }
}

void ExportDialog::prep_ui_for_render(bool r) {
  export_button->setEnabled(!r);
  cancel_button->setEnabled(!r);
  videoGroupbox->setEnabled(!r);
  audioGroupbox->setEnabled(!r);
  renderCancel->setEnabled(r);
}

void ExportDialog::StartExport() {
  if (widthSpinbox->value()%2 == 1 || heightSpinbox->value()%2 == 1) {
    QMessageBox::critical(
          this,
          tr("Invalid dimensions"),
          tr("Export width and height must both be even numbers/divisible by 2."),
          QMessageBox::Ok
          );
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
    switch (vcodecCombobox->currentData().toInt()) {
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
      qCritical() << "Invalid codec selection for an image sequence";
      QMessageBox::critical(
            this,
            tr("Invalid codec"),
            tr("Couldn't determine output parameters for the selected codec. This is a bug, please contact the developers."),
            QMessageBox::Ok
            );
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
    qCritical() << "Invalid format - this is a bug, please inform the developers";
    QMessageBox::critical(
          this,
          tr("Invalid format"),
          tr("Couldn't determine output format. This is a bug, please contact the developers."),
          QMessageBox::Ok
          );
    return;
  }
  QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Export Media"),
        "",
        format_strings[formatCombobox->currentIndex()] + " (*." + ext + ")"
      );
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

    // Set up export parameters to send to the ExportThread
    ExportParams params;
    params.filename = filename;
    params.video_enabled = videoGroupbox->isChecked();
    if (params.video_enabled) {
      params.video_codec = vcodecCombobox->currentData().toInt();
      params.video_width = widthSpinbox->value();
      params.video_height = heightSpinbox->value();
      params.video_frame_rate = framerateSpinbox->value();
      params.video_compression_type = compressionTypeCombobox->currentData().toInt();
      params.video_bitrate = videobitrateSpinbox->value();
    }
    params.audio_enabled = audioGroupbox->isChecked();
    if (params.audio_enabled) {
      params.audio_codec = acodecCombobox->currentData().toInt();
      params.audio_sampling_rate = samplingRateSpinbox->value();
      params.audio_bitrate = audiobitrateSpinbox->value();
    }

    params.start_frame = 0;
    params.end_frame = olive::ActiveSequence->getEndFrame(); // entire sequence
    if (rangeCombobox->currentIndex() == 1) {
      params.start_frame = qMax(olive::ActiveSequence->workarea_in, params.start_frame);
      params.end_frame = qMin(olive::ActiveSequence->workarea_out, params.end_frame);
    }

    // Create export thread
    export_thread_ = new ExportThread(params, vcodec_params, this);

    // Connect export thread signals/slots
    connect(export_thread_, SIGNAL(finished()), this, SLOT(export_thread_finished()));
    connect(export_thread_, SIGNAL(ProgressChanged(int, qint64)), this, SLOT(update_progress_bar(int, qint64)));
    connect(renderCancel, SIGNAL(clicked(bool)), export_thread_, SLOT(Interrupt()));

    // Close all effects in effect controls (prevents UI threading issues)
    panel_effect_controls->Clear();

    // Close all currently open clips
    olive::ActiveSequence->Close();

    olive::Global->set_export_state(true);

    olive::Global->save_autorecovery_file();

    prep_ui_for_render(true);

    total_export_time_start = QDateTime::currentMSecsSinceEpoch();

    export_thread_->start();
  }
}

void ExportDialog::update_progress_bar(int value, qint64 remaining_ms) {
  if (value == 100) {
    // if value is 100%, show total render time rather than remaining
    remaining_ms = QDateTime::currentMSecsSinceEpoch() - total_export_time_start;
  }

  // convert ms to H:MM:SS
  int seconds = qFloor(remaining_ms*0.001)%60;
  int minutes = qFloor(remaining_ms/60000)%60;
  int hours = qFloor(remaining_ms/3600000);

  if (value == 100) {
    // show value as "total"
    progressBar->setFormat(tr("%p% (Total: %1:%2:%3)").arg(QString::number(hours),
                                                            QString::number(minutes).rightJustified(2, '0'),
                                                            QString::number(seconds).rightJustified(2, '0')));
  } else {
    // show value as "remaining"
    progressBar->setFormat(tr("%p% (ETA: %1:%2:%3)").arg(QString::number(hours),
                                                         QString::number(minutes).rightJustified(2, '0'),
                                                         QString::number(seconds).rightJustified(2, '0')));
  }

  progressBar->setValue(value);
}

void ExportDialog::vcodec_changed(int index) {
  compressionTypeCombobox->clear();

  if (vcodecCombobox->count() > 0) {
    if (vcodecCombobox->itemData(index) == AV_CODEC_ID_H264
        || vcodecCombobox->itemData(index) == AV_CODEC_ID_H265) {
      compressionTypeCombobox->setEnabled(true);
      compressionTypeCombobox->addItem(tr("Quality-based (Constant Rate Factor)"), COMPRESSION_TYPE_CFR);
      //		compressionTypeCombobox->addItem("File size-based (Two-Pass)", COMPRESSION_TYPE_TARGETSIZE);
      //		compressionTypeCombobox->addItem("Average bitrate (Two-Pass)", COMPRESSION_TYPE_TARGETBR);
    } else {
      compressionTypeCombobox->addItem(tr("Constant Bitrate"), COMPRESSION_TYPE_CBR);
      compressionTypeCombobox->setCurrentIndex(0);
      compressionTypeCombobox->setEnabled(false);
    }

    // set default pix_fmt for this codec
    AVCodec* codec_info = avcodec_find_encoder(static_cast<AVCodecID>(vcodecCombobox->itemData(index).toInt()));
    if (codec_info == nullptr) {
      QMessageBox::critical(this,
                            tr("Invalid Codec"),
                            tr("Failed to find a suitable encoder for this codec. Export will likely fail."));
    } else {
      vcodec_params.pix_fmt = codec_info->pix_fmts[0];
      if (vcodec_params.pix_fmt == -1) {
        QMessageBox::critical(this,
                              tr("Invalid Codec"),
                              tr("Failed to find pixel format for this encoder. Export will likely fail."));
      }
    }
  }
}

void ExportDialog::comp_type_changed(int) {
  videobitrateSpinbox->setToolTip("");
  videobitrateSpinbox->setMinimum(0);
  videobitrateSpinbox->setMaximum(99.99);
  switch (compressionTypeCombobox->currentData().toInt()) {
  case COMPRESSION_TYPE_CBR:
  case COMPRESSION_TYPE_TARGETBR:
    videoBitrateLabel->setText(tr("Bitrate (Mbps):"));
    videobitrateSpinbox->setValue(qMax(0.5, (double) qRound((0.01528 * olive::ActiveSequence->height) - 4.5)));
    break;
  case COMPRESSION_TYPE_CFR:
    videoBitrateLabel->setText(tr("Quality (CRF):"));
    videobitrateSpinbox->setValue(23);
    videobitrateSpinbox->setMaximum(51);
    videobitrateSpinbox->setToolTip(tr("Quality Factor:\n\n0 = lossless\n17-18 = visually lossless (compressed, but unnoticeable)\n23 = high quality\n51 = lowest quality possible"));
    break;
  case COMPRESSION_TYPE_TARGETSIZE:
    videoBitrateLabel->setText(tr("Target File Size (MB):"));
    videobitrateSpinbox->setValue(100);
    break;
  }
}

void ExportDialog::open_advanced_video_dialog() {
  AdvancedVideoDialog avd(this, static_cast<AVCodecID>(vcodecCombobox->currentData().toInt()), vcodec_params);
  avd.exec();
}

void ExportDialog::setup_ui() {
  QVBoxLayout* verticalLayout = new QVBoxLayout(this);

  QHBoxLayout* format_layout = new QHBoxLayout();

  format_layout->addWidget(new QLabel(tr("Format:"), this));

  formatCombobox = new QComboBox();
  format_layout->addWidget(formatCombobox);

  verticalLayout->addLayout(format_layout);

  QHBoxLayout* range_layout = new QHBoxLayout();

  range_layout->addWidget(new QLabel(tr("Range:"), this));

  rangeCombobox = new QComboBox(this);
  rangeCombobox->addItem(tr("Entire Sequence"));
  rangeCombobox->addItem(tr("In to Out"));

  range_layout->addWidget(rangeCombobox);

  verticalLayout->addLayout(range_layout);

  videoGroupbox = new QGroupBox(this);
  videoGroupbox->setTitle(tr("Video"));
  videoGroupbox->setFlat(false);
  videoGroupbox->setCheckable(true);

  QGridLayout* videoGridLayout = new QGridLayout(videoGroupbox);

  videoGridLayout->addWidget(new QLabel(tr("Codec:"), this), 0, 0, 1, 1);
  vcodecCombobox = new QComboBox(videoGroupbox);
  videoGridLayout->addWidget(vcodecCombobox, 0, 1, 1, 1);

  videoGridLayout->addWidget(new QLabel(tr("Width:"), this), 1, 0, 1, 1);
  widthSpinbox = new QSpinBox(videoGroupbox);
  widthSpinbox->setMaximum(16777216);
  videoGridLayout->addWidget(widthSpinbox, 1, 1, 1, 1);

  videoGridLayout->addWidget(new QLabel(tr("Height:"), this), 2, 0, 1, 1);
  heightSpinbox = new QSpinBox(videoGroupbox);
  heightSpinbox->setMaximum(16777216);
  videoGridLayout->addWidget(heightSpinbox, 2, 1, 1, 1);

  videoGridLayout->addWidget(new QLabel(tr("Frame Rate:"), this), 3, 0, 1, 1);
  framerateSpinbox = new QDoubleSpinBox(videoGroupbox);
  framerateSpinbox->setMaximum(60);
  framerateSpinbox->setValue(0);
  videoGridLayout->addWidget(framerateSpinbox, 3, 1, 1, 1);

  videoGridLayout->addWidget(new QLabel(tr("Compression Type:"), this), 4, 0, 1, 1);
  compressionTypeCombobox = new QComboBox(videoGroupbox);
  videoGridLayout->addWidget(compressionTypeCombobox, 4, 1, 1, 1);

  videoBitrateLabel = new QLabel(videoGroupbox);
  videoGridLayout->addWidget(videoBitrateLabel, 5, 0, 1, 1);
  videobitrateSpinbox = new QDoubleSpinBox(videoGroupbox);
  videobitrateSpinbox->setMaximum(100);
  videobitrateSpinbox->setValue(2);
  videoGridLayout->addWidget(videobitrateSpinbox, 5, 1, 1, 1);

  QPushButton* advanced_video_button = new QPushButton(tr("Advanced"));
  connect(advanced_video_button, SIGNAL(clicked(bool)), this, SLOT(open_advanced_video_dialog()));
  videoGridLayout->addWidget(advanced_video_button, 6, 1);

  verticalLayout->addWidget(videoGroupbox);

  audioGroupbox = new QGroupBox(this);
  audioGroupbox->setTitle(tr("Audio"));
  audioGroupbox->setCheckable(true);

  QGridLayout* audioGridLayout = new QGridLayout(audioGroupbox);

  audioGridLayout->addWidget(new QLabel(tr("Codec:"), this), 0, 0, 1, 1);
  acodecCombobox = new QComboBox(audioGroupbox);
  audioGridLayout->addWidget(acodecCombobox, 0, 1, 1, 1);

  audioGridLayout->addWidget(new QLabel(tr("Sampling Rate:"), this), 1, 0, 1, 1);
  samplingRateSpinbox = new QSpinBox(audioGroupbox);
  samplingRateSpinbox->setMaximum(96000);
  samplingRateSpinbox->setValue(0);
  audioGridLayout->addWidget(samplingRateSpinbox, 1, 1, 1, 1);

  audioGridLayout->addWidget(new QLabel(tr("Bitrate (Kbps/CBR):"), this), 3, 0, 1, 1);
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
  renderCancel->setIcon(QIcon(":/icons/error.svg"));
  renderCancel->setEnabled(false);
  progressLayout->addWidget(renderCancel);

  verticalLayout->addLayout(progressLayout);

  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();

  export_button = new QPushButton(this);
  export_button->setText("Export");
  connect(export_button, SIGNAL(clicked(bool)), this, SLOT(StartExport()));

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
