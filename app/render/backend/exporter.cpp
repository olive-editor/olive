#include "exporter.h"

#include "render/colormanager.h"
#include "render/pixelservice.h"

Exporter::Exporter(ViewerOutput* viewer,
                   const VideoRenderingParams& video_params,
                   const AudioRenderingParams& audio_params,
                   const QMatrix4x4 &transform,
                   ColorProcessorPtr color_processor,
                   Encoder *encoder,
                   QObject* parent) :
  QObject(parent),
  video_backend_(nullptr),
  audio_backend_(nullptr),
  viewer_node_(viewer),
  video_params_(video_params),
  audio_params_(audio_params),
  transform_(transform),
  color_processor_(color_processor),
  encoder_(encoder),
  export_status_(false),
  export_msg_(tr("Export hasn't started yet"))
{
  connect(this, SIGNAL(ExportEnded()), this, SLOT(deleteLater()));
}

bool Exporter::GetExportStatus() const
{
  return export_status_;
}

const QString &Exporter::GetExportError() const
{
  return export_msg_;
}

void Exporter::StartExporting()
{
  // Default to error state until ExportEnd is called
  export_status_ = false;

  // Create renderers
  if (!Initialize()) {
    SetExportMessage("Failed to initialize exporter");
    ExportFailed();
    return;
  }

  video_backend_->SetViewerNode(viewer_node_);
  video_backend_->SetParameters(VideoRenderingParams(viewer_node_->video_params().width(),
                                                     viewer_node_->video_params().height(),
                                                     video_params_.time_base(),
                                                     video_params_.format(),
                                                     video_params_.mode()));
  video_backend_->SetExportMode(true);
  audio_backend_->SetViewerNode(viewer_node_);
  audio_backend_->SetParameters(audio_params_);

  // Connect to renderers
  connect(video_backend_, SIGNAL(CachedFrameReady(const rational&, QVariant)), this, SLOT(FrameRendered(const rational&, QVariant)));
  connect(audio_backend_, SIGNAL(QueueComplete()), this, SLOT(AudioRendered()));

  // Create renderers
  waiting_for_frame_ = 0;

  // Open encoder and wait for result
  connect(encoder_, SIGNAL(OpenSucceeded()), this, SLOT(EncoderOpenedSuccessfully()), Qt::QueuedConnection);
  connect(encoder_, SIGNAL(OpenFailed()), this, SLOT(EncoderOpenFailed()), Qt::QueuedConnection);
  connect(encoder_, SIGNAL(Closed()), encoder_, SLOT(deleteLater()), Qt::QueuedConnection);

  QMetaObject::invokeMethod(encoder_,
                            "Open",
                            Qt::QueuedConnection);
}

void Exporter::SetExportMessage(const QString &s)
{
  export_msg_ = s;
}

void Exporter::ExportSucceeded()
{
  Cleanup();

  video_backend_->deleteLater();

  export_status_ = true;

  connect(encoder_, SIGNAL(Closed()), this, SLOT(EncoderClosed()));

  QMetaObject::invokeMethod(encoder_,
                            "Close",
                            Qt::QueuedConnection);
}

void Exporter::ExportFailed()
{
  emit ExportEnded();
}

void Exporter::FrameRendered(const rational &time, QVariant value)
{
  qDebug() << "Received" << time.toDouble() << "- waiting for" << waiting_for_frame_.toDouble();

  if (time == waiting_for_frame_) {
    bool get_cached = false;

    do {
      if (get_cached) {
        value = cached_frames_.take(waiting_for_frame_);
      } else {
        get_cached = true;
      }

      // Convert texture to frame
      FramePtr frame = TextureToFrame(value);

      // OCIO conversion requires a frame in 32F format
      if (frame->format() != olive::PIX_FMT_RGBA32F) {
        frame = PixelService::ConvertPixelFormat(frame, olive::PIX_FMT_RGBA32F);
      }

      // Color conversion must be done with unassociated alpha, and the pipeline is always associated
      ColorManager::DisassociateAlpha(frame);

      // Convert color space
      color_processor_->ConvertFrame(frame);

      // Set frame timestamp
      frame->set_timestamp(waiting_for_frame_);

      // Encode (may require re-associating alpha?)
      QMetaObject::invokeMethod(encoder_,
                                "Write",
                                Qt::QueuedConnection,
                                Q_ARG(FramePtr, frame));

      waiting_for_frame_ += video_params_.time_base();

      // Calculate progress
      int progress = qRound(100.0 * (waiting_for_frame_.toDouble() / viewer_node_->Length().toDouble()));
      emit ProgressChanged(progress);

    } while (cached_frames_.contains(waiting_for_frame_));

    if (waiting_for_frame_ >= viewer_node_->Length()) {
      ExportSucceeded();
    }
  } else {
    cached_frames_.insert(time, value);
  }
}

void Exporter::AudioRendered()
{
  // Retrieve the audio filename
  QString cache_fn = audio_backend_->CachePathName();

  QMetaObject::invokeMethod(encoder_,
                            "WriteAudio",
                            Qt::QueuedConnection,
                            Q_ARG(const AudioRenderingParams&, audio_backend_->params()),
                            Q_ARG(const QString&, cache_fn));

  // We don't need the audio backend anymore
  audio_backend_->deleteLater();
}

void Exporter::EncoderOpenedSuccessfully()
{
  // Invalidate caches
  if (encoder_->params().video_enabled()) {
    video_backend_->InvalidateCache(0, viewer_node_->Length());
  }

  if (encoder_->params().audio_enabled()) {
    audio_backend_->InvalidateCache(0, viewer_node_->Length());
  }
}

void Exporter::EncoderOpenFailed()
{
  SetExportMessage("Failed to open encoder");
  ExportFailed();
}

void Exporter::EncoderClosed()
{
  emit ProgressChanged(100);
  emit ExportEnded();
}
