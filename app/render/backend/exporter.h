/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef EXPORTER_H
#define EXPORTER_H

#include <QMatrix4x4>
#include <QString>
#include <QTimer>
#include <QObject>

#include "codec/encoder.h"
#include "node/output/viewer/viewer.h"
#include "render/backend/audiorenderbackend.h"
#include "render/backend/exportparams.h"
#include "render/backend/videorenderbackend.h"
#include "render/colorprocessor.h"

OLIVE_NAMESPACE_ENTER

class Exporter : public QObject
{
  Q_OBJECT
public:
  Exporter(ViewerOutput* viewer_node,
           ColorManager* color_manager,
           const ExportParams& params,
           QObject* parent = nullptr);

  bool GetExportStatus() const;
  const QString& GetExportError() const;

  void Cancel();

  static QMatrix4x4 GenerateMatrix(ExportParams::VideoScalingMethod method, int source_width, int source_height, int dest_width, int dest_height);

public slots:
  void StartExporting();

signals:
  void ProgressChanged(double);

  void ExportEnded();

protected:
  void SetExportMessage(const QString& s);

private:
  void ExportSucceeded();

  void ExportStopped();

  void EncodeFrame();

  ViewerOutput* viewer_node_;

  ColorProcessorPtr color_processor_;

  ExportParams params_;

  // Renderers
  VideoRenderBackend* video_backend_;
  AudioRenderBackend* audio_backend_;

  // Export transform
  QMatrix4x4 transform_;

  bool video_done_;

  bool audio_done_;

  Encoder* encoder_;

  bool export_status_;

  QString export_msg_;

  TimeRange export_range_;

  rational waiting_for_frame_;

  QHash<rational, FramePtr> cached_frames_;

  QTimer debug_timer_;

private slots:
  void FrameRendered(FramePtr frame);

  void AudioRendered();

  void AudioEncodeComplete();

  void EncoderOpenedSuccessfully();

  void EncoderOpenFailed();

  void EncoderClosed();

  void VideoHashesComplete();

  void DebugTimerMessage();

  void FrameColorFinished();

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTER_H
