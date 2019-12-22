#ifndef EXPORTER_H
#define EXPORTER_H

#include <QMatrix4x4>
#include <QString>
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
           const VideoRenderingParams& video_params,
           const AudioRenderingParams& audio_params,
           const QMatrix4x4& transform,
           ColorProcessorPtr color_processor,
           Encoder* encoder,
           QObject* parent = nullptr);

  bool GetExportStatus() const;
  const QString& GetExportError() const;

public slots:
  void StartExporting();

signals:
  void ProgressChanged(int);

  void ExportEnded();

protected:
  virtual bool Initialize() = 0;
  virtual void Cleanup() = 0;

  virtual FramePtr TextureToFrame(const QVariant &texture) = 0;

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

private:
  void ExportSucceeded();

  void ExportFailed();

  ColorProcessorPtr color_processor_;

  Encoder* encoder_;

  bool export_status_;

  QString export_msg_;

  rational waiting_for_frame_;

  QHash<rational, QVariant> cached_frames_;

private slots:
  void FrameRendered(const rational& time, QVariant value);

  void AudioRendered();

  void EncoderOpenedSuccessfully();

  void EncoderOpenFailed();

  void EncoderClosed();

};

#endif // EXPORTER_H
