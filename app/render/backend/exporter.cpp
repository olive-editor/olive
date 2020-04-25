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

#include "exporter.h"

#include "render/backend/audio/audiobackend.h"
#include "render/backend/opengl/openglbackend.h"
#include "render/colormanager.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

Exporter::Exporter(ViewerOutput *viewer_node,
                   ColorManager *color_manager,
                   const ExportParams& params,
                   QObject* parent) :
  QObject(parent),
  viewer_node_(viewer_node),
  params_(params),
  video_backend_(nullptr),
  audio_backend_(nullptr),
  export_status_(false),
  export_msg_(tr("Export hasn't started yet"))
{
  encoder_ = Encoder::CreateFromID(params_.encoder(), params_);

  video_done_ = !params_.video_enabled();
  audio_done_ = !params_.audio_enabled();

  debug_timer_.setInterval(5000);
  connect(&debug_timer_, &QTimer::timeout, this, &Exporter::DebugTimerMessage);

  connect(this, &Exporter::ExportEnded, this, &Exporter::deleteLater);

  if (params_.has_custom_range()) {
    export_range_ = params_.custom_range();
  } else {
    export_range_ = TimeRange(0, viewer_node_->Length());
  }

  if (params_.video_enabled()) {

    // If a transformation matrix is applied to this video, create it here
    if (params_.video_scaling_method() != ExportParams::kStretch) {
      transform_ = GenerateMatrix(params_.video_scaling_method(),
                                  viewer_node_->video_params().width(),
                                  viewer_node_->video_params().height(),
                                  params_.video_params().width(),
                                  params_.video_params().height());
    }

    // Create color processor
    color_processor_ = ColorProcessor::Create(color_manager,
                                              color_manager->GetReferenceColorSpace(),
                                              params.color_transform());
  }
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
    video_backend_->Close();
    video_backend_->deleteLater();
    video_backend_ = nullptr;
  }

  if (audio_backend_) {
    audio_backend_->CancelQueue();
    audio_backend_->Close();
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
                                                       params_.video_params().time_base(),
                                                       params_.video_params().format(),
                                                       params_.video_params().mode()));

    waiting_for_frame_ = 0;
  }

  if (!audio_done_) {
    audio_backend_ = new AudioBackend();

    audio_backend_->SetViewerNode(viewer_node_);
    audio_backend_->SetParameters(params_.audio_params());
  }

  // Open encoder and wait for result
  connect(encoder_, &Encoder::OpenSucceeded, this, &Exporter::EncoderOpenedSuccessfully, Qt::QueuedConnection);
  connect(encoder_, &Encoder::OpenFailed, this, &Exporter::EncoderOpenFailed, Qt::QueuedConnection);
  connect(encoder_, &Encoder::AudioComplete, this, &Exporter::AudioEncodeComplete, Qt::QueuedConnection);

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
    video_backend_->Close();
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

    // Encode (may require re-associating alpha?)
    QMetaObject::invokeMethod(encoder_,
                              "WriteFrame",
                              Qt::QueuedConnection,
                              OLIVE_NS_ARG(FramePtr, frame),
                              OLIVE_NS_ARG(rational, waiting_for_frame_));

    waiting_for_frame_ += params_.video_params().time_base();

    // Calculate progress
    emit ProgressChanged(waiting_for_frame_.toDouble() / viewer_node_->Length().toDouble());
  }

  if (waiting_for_frame_ >= viewer_node_->Length()) {
    video_done_ = true;
    debug_timer_.stop();

    ExportSucceeded();
  }
}

QMatrix4x4 Exporter::GenerateMatrix(ExportParams::VideoScalingMethod method, int source_width, int source_height, int dest_width, int dest_height)
{
  QMatrix4x4 preview_matrix;

  if (method == ExportParams::kStretch) {
    return preview_matrix;
  }

  float export_ar = static_cast<float>(dest_width) / static_cast<float>(dest_height);
  float source_ar = static_cast<float>(source_width) / static_cast<float>(source_height);

  if (qFuzzyCompare(export_ar, source_ar)) {
    return preview_matrix;
  }

  if ((export_ar > source_ar) == (method == ExportParams::kFit)) {
    preview_matrix.scale(source_ar / export_ar, 1.0F);
  } else {
    preview_matrix.scale(1.0F, export_ar / source_ar);
  }

  return preview_matrix;
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

  debug_timer_.start();

  EncodeFrame();
}

void Exporter::AudioRendered()
{
  // Retrieve the audio filename
  QString cache_fn = audio_backend_->CachePathName();

  QMetaObject::invokeMethod(encoder_,
                            "WriteAudio",
                            Qt::QueuedConnection,
                            OLIVE_NS_ARG(AudioRenderingParams, audio_backend_->params()),
                            Q_ARG(const QString&, cache_fn),
                            OLIVE_NS_ARG(TimeRange, export_range_));

  // We don't need the audio backend anymorea
  audio_backend_->Close();
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

    video_backend_->InvalidateCache(export_range_, nullptr);
  }

  if (!audio_done_) {
    // We set the audio backend to render the full sequence to the disk
    connect(audio_backend_, &AudioRenderBackend::AudioComplete, this, &Exporter::AudioRendered);

    audio_backend_->InvalidateCache(export_range_, nullptr);
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
  video_backend_->SetFrameGenerationParams(params_.video_params().width(), params_.video_params().height(), transform_);

  connect(video_backend_, &VideoRenderBackend::GeneratedFrame, this, &Exporter::FrameRendered);

  // Remove duplicate frames from cache invalidation
  const QMap<rational, QByteArray>& time_hash_map = video_backend_->frame_cache()->time_hash_map();
  QList<QByteArray> hashes_already_seen;
  QMap<rational, QByteArray>::const_iterator i;

  for (i=time_hash_map.begin(); i!=time_hash_map.end(); i++) {
    if (hashes_already_seen.contains(i.value())) {
      ranges.RemoveTimeRange(TimeRange(i.key(), i.key() + params_.video_params().time_base()));
    } else {
      hashes_already_seen.append(i.value());
    }
  }

  foreach (const TimeRange& range, ranges) {
    video_backend_->InvalidateCache(range, nullptr);
  }
}

void Exporter::DebugTimerMessage()
{
  qDebug() << "Still waiting for" << waiting_for_frame_.toDouble();
}

OLIVE_NAMESPACE_EXIT
