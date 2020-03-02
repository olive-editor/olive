#include "exporter.h"

#include "render/backend/audio/audiobackend.h"
#include "render/backend/opengl/openglbackend.h"
#include "render/colormanager.h"
#include "render/pixelformat.h"

Exporter::Exporter(ViewerOutput* viewer,
                   Encoder *encoder,
                   QObject* parent) :
  QObject(parent),
  video_backend_(nullptr),
  audio_backend_(nullptr),
  viewer_node_(viewer),
  video_done_(true),
  audio_done_(true),
  encoder_(encoder),
  export_status_(false),
  export_msg_(tr("Export hasn't started yet"))
{
  debug_timer_.setInterval(5000);
  connect(&debug_timer_, &QTimer::timeout, this, &Exporter::DebugTimerMessage);

  connect(this, &Exporter::ExportEnded, this, &Exporter::deleteLater);
}

void Exporter::EnableVideo(const VideoRenderingParams &video_params, const QMatrix4x4 &transform, ColorProcessorPtr color_processor)
{
  video_params_ = video_params;
  transform_ = transform;
  color_processor_ = color_processor;

  video_done_ = false;
}

void Exporter::EnableAudio(const AudioRenderingParams &audio_params)
{
  audio_params_ = audio_params;

  audio_done_ = false;
}

bool Exporter::GetExportStatus() const
{
  return export_status_;
}

const QString &Exporter::GetExportError() const
{
  return export_msg_;
}

void Exporter::Cancel()
{
  if (video_backend_) {
    video_backend_->CancelQueue();
    video_backend_->deleteLater();
    video_backend_ = nullptr;
  }

  if (audio_backend_) {
    audio_backend_->CancelQueue();
    audio_backend_->deleteLater();
    audio_backend_ = nullptr;
  }

  SetExportMessage(tr("User cancelled export"));
  ExportStopped();
}

void Exporter::StartExporting()
{
  // Default to error state until ExportEnd is called
  export_status_ = false;

  // Create renderers
  if (!video_done_) {
    video_backend_ = new OpenGLBackend();

    video_backend_->SetLimitCaching(false);
    video_backend_->SetViewerNode(viewer_node_);
    video_backend_->SetParameters(VideoRenderingParams(viewer_node_->video_params().width(),
                                                       viewer_node_->video_params().height(),
                                                       video_params_.time_base(),
                                                       video_params_.format(),
                                                       video_params_.mode()));

    waiting_for_frame_ = 0;
  }

  if (!audio_done_) {
    audio_backend_ = new AudioBackend();

    audio_backend_->SetViewerNode(viewer_node_);
    audio_backend_->SetParameters(audio_params_);
  }

  // Open encoder and wait for result
  connect(encoder_, &Encoder::OpenSucceeded, this, &Exporter::EncoderOpenedSuccessfully, Qt::QueuedConnection);
  connect(encoder_, &Encoder::OpenFailed, this, &Exporter::EncoderOpenFailed, Qt::QueuedConnection);
  connect(encoder_, &Encoder::AudioComplete, this, &Exporter::AudioEncodeComplete, Qt::QueuedConnection);
  connect(encoder_, &Encoder::Closed, encoder_, &Encoder::deleteLater, Qt::QueuedConnection);

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
  if (!audio_done_ || !video_done_) {
    return;
  }

  if (video_backend_) {
    video_backend_->deleteLater();
    video_backend_ = nullptr;
  }

  export_status_ = true;

  connect(encoder_, &Encoder::Closed, this, &Exporter::EncoderClosed);

  QMetaObject::invokeMethod(encoder_,
                            "Close",
                            Qt::QueuedConnection);
}

void Exporter::ExportStopped()
{
  emit ExportEnded();
  encoder_->deleteLater();
}

void Exporter::EncodeFrame()
{
  while (cached_frames_.contains(waiting_for_frame_)) {
    FramePtr frame = cached_frames_.take(waiting_for_frame_);

    // OCIO conversion requires a frame in 32F format
    if (frame->format() != PixelFormat::PIX_FMT_RGBA32F) {
      frame = PixelFormat::ConvertPixelFormat(frame, PixelFormat::PIX_FMT_RGBA32F);
    }

    // Color conversion must be done with unassociated alpha, and the pipeline is always associated
    ColorManager::DisassociateAlpha(frame);

    // Convert color space
    color_processor_->ConvertFrame(frame);

    // Set frame timestamp
    frame->set_timestamp(waiting_for_frame_);

    // Encode (may require re-associating alpha?)
    QMetaObject::invokeMethod(encoder_,
                              "WriteFrame",
                              Qt::QueuedConnection,
                              Q_ARG(FramePtr, frame));

    waiting_for_frame_ += video_params_.time_base();

    // Calculate progress
    int progress = qRound(100.0 * (waiting_for_frame_.toDouble() / viewer_node_->Length().toDouble()));
    emit ProgressChanged(progress);
  }

  if (waiting_for_frame_ >= viewer_node_->Length()) {
    video_done_ = true;
    debug_timer_.stop();

    ExportSucceeded();
  }
}

void Exporter::FrameRendered(const rational &time, FramePtr value)
{
  debug_timer_.stop();

  const QMap<rational, QByteArray>& time_hash_map = video_backend_->frame_cache()->time_hash_map();

  QByteArray this_hash = time_hash_map.value(time);

  qDebug() << "Received" << this_hash.toHex();

  QList<rational> matching_times = time_hash_map.keys(this_hash);

  foreach (const rational& t, matching_times) {
    qDebug() << "  Matches" << t.toDouble();

    cached_frames_.insert(t, value);
  }

  qDebug() << "    Waiting for" << waiting_for_frame_.toDouble();

  EncodeFrame();

  debug_timer_.start();
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
  audio_backend_ = nullptr;
}

void Exporter::AudioEncodeComplete()
{
  audio_done_ = true;

  ExportSucceeded();
}

void Exporter::EncoderOpenedSuccessfully()
{
  // Invalidate caches
  if (!video_done_) {
    // First we generate the hashes so we know exactly how many frames we need
    video_backend_->SetOperatingMode(VideoRenderWorker::kHashOnly);
    connect(video_backend_, &VideoRenderBackend::QueueComplete, this, &Exporter::VideoHashesComplete);

    video_backend_->InvalidateCache(TimeRange(0, viewer_node_->Length()));
  }

  if (!audio_done_) {
    // We set the audio backend to render the full sequence to the disk
    connect(audio_backend_, &AudioRenderBackend::QueueComplete, this, &Exporter::AudioRendered);

    audio_backend_->InvalidateCache(TimeRange(0, viewer_node_->Length()));
  }
}

void Exporter::EncoderOpenFailed()
{
  SetExportMessage(tr("Failed to open encoder"));
  ExportStopped();
}

void Exporter::EncoderClosed()
{
  emit ProgressChanged(100);
  ExportStopped();
}

void Exporter::VideoHashesComplete()
{
  // We've got our hashes, time to kick off actual rendering
  disconnect(video_backend_, &VideoRenderBackend::QueueComplete, this, &Exporter::VideoHashesComplete);

  // Determine what frames will be hashed
  TimeRangeList ranges;
  ranges.append(TimeRange(0, viewer_node_->Length()));

  // Set video backend to render mode but NOT hash or download
  video_backend_->SetOperatingMode(VideoRenderWorker::kRenderOnly);
  video_backend_->SetOnlySignalLastFrameRequested(false);

  // FIXME: Exporting is now broken because of this
  connect(video_backend_, &VideoRenderBackend::GeneratedFrame, this, &Exporter::FrameRendered);

  foreach (const TimeRange& range, ranges) {
    video_backend_->InvalidateCache(range);
  }
}

void Exporter::DebugTimerMessage()
{
  qDebug() << "Still waiting for" << waiting_for_frame_.toDouble();
}
