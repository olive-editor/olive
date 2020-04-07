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
#include "render/backend/videorenderbackend.h"
#include "render/colorprocessor.h"

OLIVE_NAMESPACE_ENTER

class Exporter : public QObject
{
  Q_OBJECT
public:
  Exporter(ViewerOutput* viewer,
           Encoder* encoder,
           QObject* parent = nullptr);

  void EnableVideo(const VideoRenderingParams& video_params, const QMatrix4x4& transform, ColorProcessorPtr color_processor);
  void EnableAudio(const AudioRenderingParams& audio_params);

  void OverrideExportRange(const TimeRange& range);

  bool GetExportStatus() const;
  const QString& GetExportError() const;

  void Cancel();

public slots:
  void StartExporting();

signals:
  void ProgressChanged(double);

  void ExportEnded();

protected:
  void SetExportMessage(const QString& s);

  // Renderers
  VideoRenderBackend* video_backend_;
  AudioRenderBackend* audio_backend_;

  // Viewer node
  ViewerOutput* viewer_node_;

  // Export parameters
  VideoRenderingParams video_params_;
  AudioRenderingParams audio_params_;

  // Export transform
  QMatrix4x4 transform_;

  bool video_done_;

  bool audio_done_;

  TimeRange export_range_;

private:
  void ExportSucceeded();

  void ExportStopped();

  void EncodeFrame();

  ColorProcessorPtr color_processor_;

  Encoder* encoder_;

  bool export_status_;

  QString export_msg_;

  rational waiting_for_frame_;

  QHash<rational, FramePtr> cached_frames_;

  QTimer debug_timer_;

private slots:
  void FrameRendered(const rational &time, FramePtr value);

  void AudioRendered();

  void AudioEncodeComplete();

  void EncoderOpenedSuccessfully();

  void EncoderOpenFailed();

  void EncoderClosed();

  void VideoHashesComplete();

  void DebugTimerMessage();

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTER_H
