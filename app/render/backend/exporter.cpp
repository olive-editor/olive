#include "exporter.h"

#include "render/colormanager.h"
#include "render/pixelservice.h"

Exporter::Exporter(ViewerOutput* viewer, const VideoRenderingParams& params, const QMatrix4x4 &transform, ColorProcessorPtr color_processor, QObject* parent) :
  QObject(parent),
  video_backend_(nullptr),
  audio_backend_(nullptr),
  viewer_node_(viewer),
  params_(params),
  color_processor_(color_processor),
  export_status_(false),
  export_msg_(tr("Export hasn't started yet"))
{
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
    ExportFailed();
    return;
  }

  video_backend_->SetViewerNode(viewer_node_);
  video_backend_->SetParameters(VideoRenderingParams(viewer_node_->video_params(), olive::PIX_FMT_RGBA32F, olive::kOnline));
  video_backend_->SetExportMode(true);
  video_backend_->Compile();
  //audio_backend_->SetViewerNode(viewer_node_);
  //audio_renderer_->SetParameters(AudioRenderingParams(viewer_node_->audio_params(), SAMPLE_FMT_FLT));

  // Connect to renderers
  connect(video_backend_, SIGNAL(CachedFrameReady(const rational&, QVariant)), this, SLOT(FrameRendered(const rational&, QVariant)));
  connect(video_backend_, SIGNAL(CachedTimeReady(const rational&)), this, SLOT(TimeRendered(const rational&)));

  // Create renderers
  waiting_for_frame_ = 0;

  // Invalidate caches
  video_backend_->InvalidateCache(0, viewer_node_->Length());
  //viewer_node_->InvalidateCache(0, viewer_node_->Length(), viewer_node_->samples_input());
}

void Exporter::SetExportMessage(const QString &s)
{
  export_msg_ = s;
}

void Exporter::ExportSucceeded()
{
  Cleanup();

  delete video_backend_;
  delete audio_backend_;

  export_status_ = true;
  export_msg_ = tr("Export succeeded");

  emit ExportEnded();
}

void Exporter::ExportFailed()
{
  emit ExportEnded();
}

void Exporter::FrameRendered(const rational &time, QVariant value)
{
  qDebug() << "Retrieved frame" << time.toDouble() << "- waiting for frame" << waiting_for_frame_.toDouble();

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

      // Encode (may require re-associating alpha?)

      waiting_for_frame_ += viewer_node_->video_params().time_base();

      // Calculate progress
      int progress = qRound(100.0 * (waiting_for_frame_.toDouble() / viewer_node_->Length().toDouble()));
      qDebug() << "Hello progress" << progress;
      emit ProgressChanged(progress);

    } while (cached_frames_.contains(waiting_for_frame_));
  } else {
    cached_frames_.insert(time, value);
  }

  /*if (time == viewer_node_->Length()) {
    ExportSucceeded();
  }*/
}

void Exporter::TimeRendered(const rational &time)
{
  qDebug() << "Retrieved time rendered!" << time.toDouble();
}
