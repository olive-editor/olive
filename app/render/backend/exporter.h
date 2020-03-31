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
  void ProgressChanged(int);

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

#endif // EXPORTER_H
