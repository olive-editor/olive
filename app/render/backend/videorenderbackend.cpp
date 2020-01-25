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

#include "videorenderbackend.h"

#include <OpenImageIO/imageio.h>
#include <QApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QtMath>

#include "common/timecodefunctions.h"
#include "config/config.h"
#include "render/diskmanager.h"
#include "render/diskmanager.h"
#include "render/pixelservice.h"
#include "videorenderworker.h"

VideoRenderBackend::VideoRenderBackend(QObject *parent) :
  RenderBackend(parent),
  operating_mode_(VideoRenderWorker::kHashRenderCache),
  only_signal_last_frame_requested_(true)
{
  connect(DiskManager::instance(), &DiskManager::DeletedFrame, this, &VideoRenderBackend::FrameRemovedFromDiskCache);
}

bool VideoRenderBackend::InitInternal()
{
  ResizeCacheLoadBuffer();
  return true;
}

void VideoRenderBackend::CloseInternal()
{
  cache_frame_load_buffer_.clear();
}

void VideoRenderBackend::ConnectViewer(ViewerOutput *node)
{
  connect(node, &ViewerOutput::VideoChangedBetween, this, &VideoRenderBackend::InvalidateCache);
  connect(node, &ViewerOutput::VideoGraphChanged, this, &VideoRenderBackend::QueueRecompile);
  connect(node, &ViewerOutput::LengthChanged, this, &VideoRenderBackend::TruncateFrameCacheLength);
}

void VideoRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, &ViewerOutput::VideoChangedBetween, this, &VideoRenderBackend::InvalidateCache);
  disconnect(node, &ViewerOutput::VideoGraphChanged, this, &VideoRenderBackend::QueueRecompile);
  disconnect(node, &ViewerOutput::LengthChanged, this, &VideoRenderBackend::TruncateFrameCacheLength);

  frame_cache_.Clear();
}

const VideoRenderingParams &VideoRenderBackend::params() const
{
  return params_;
}

void VideoRenderBackend::SetParameters(const VideoRenderingParams& params)
{
  CancelQueue();

  // Set new parameters
  params_ = params;

  // Resize frame load buffer
  ResizeCacheLoadBuffer();

  // Handle custom events from derivatives
  ParamsChangedEvent();

  // Set params on all processors
  foreach (RenderWorker* worker, processors_) {
    static_cast<VideoRenderWorker*>(worker)->SetParameters(params_);
  }

  // Regenerate the cache ID
  RegenerateCacheID();
}

void VideoRenderBackend::SetOperatingMode(const VideoRenderWorker::OperatingMode &mode)
{
  if (!AllProcessorsAreAvailable()) {
    qCritical() << "Attempted to set operating mode on a backend whose workers are still busy";
    return;
  }

  operating_mode_ = mode;

  foreach (RenderWorker* worker, processors_) {
    static_cast<VideoRenderWorker*>(worker)->SetOperatingMode(operating_mode_);
  }
}

void VideoRenderBackend::SetOnlySignalLastFrameRequested(bool enabled)
{
  only_signal_last_frame_requested_ = enabled;
}

bool VideoRenderBackend::IsRendered(const rational &time) const
{
  TimeRange range(time, time);

  return !TimeIsQueued(range) && !render_job_info_.contains(range);
}

bool VideoRenderBackend::GenerateCacheIDInternal(QCryptographicHash& hash)
{
  if (!params_.is_valid()) {
    return false;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  hash.addData(QString::number(params_.width()).toUtf8());
  hash.addData(QString::number(params_.height()).toUtf8());
  hash.addData(QString::number(params_.format()).toUtf8());
  hash.addData(QString::number(params_.divider()).toUtf8());

  return true;
}

void VideoRenderBackend::CacheIDChangedEvent(const QString &id)
{
  frame_cache_.SetCacheID(id);
}

void VideoRenderBackend::ConnectWorkerToThis(RenderWorker *processor)
{
  VideoRenderWorker* video_processor = static_cast<VideoRenderWorker*>(processor);

  video_processor->SetOperatingMode(operating_mode_);

  connect(video_processor, &VideoRenderWorker::CompletedFrame, this, &VideoRenderBackend::ThreadCompletedFrame, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::HashAlreadyBeingCached, this, &VideoRenderBackend::ThreadSkippedFrame, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::CompletedDownload, this, &VideoRenderBackend::ThreadCompletedDownload, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::HashAlreadyExists, this, &VideoRenderBackend::ThreadHashAlreadyExists, Qt::QueuedConnection);
}

void VideoRenderBackend::InvalidateCacheInternal(const rational &start_range, const rational &end_range)
{
  TimeRange invalidated(start_range, end_range);

  invalidated_.InsertTimeRange(invalidated);

  emit RangeInvalidated(invalidated);

  Requeue();
}

void VideoRenderBackend::ParamsChangedEvent()
{
}

VideoRenderFrameCache *VideoRenderBackend::frame_cache()
{
  return &frame_cache_;
}

const char *VideoRenderBackend::GetCachedFrame(const rational &time)
{
  last_time_requested_ = time;

  if (viewer_node() == nullptr) {
    // Nothing is connected - nothing to show or render
    return nullptr;
  }

  if (cache_id().isEmpty()) {
    qWarning() << "No cache ID";
    return nullptr;
  }

  if (!params_.is_valid()) {
    qWarning() << "Invalid parameters";
    return nullptr;
  }

  Requeue();

  // Find frame in map
  QByteArray frame_hash = frame_cache_.TimeToHash(time);

  if (!frame_hash.isEmpty()) {
    QString fn = frame_cache_.CachePathName(frame_hash, params_.format());

    if (QFileInfo::exists(fn)) {
      auto in = OIIO::ImageInput::open(fn.toStdString());

      if (in) {
        DiskManager::instance()->Accessed(frame_hash);

        in->read_image(PixelService::GetPixelFormatInfo(params_.format()).oiio_desc, cache_frame_load_buffer_.data());

        in->close();

        return cache_frame_load_buffer_.constData();
      } else {
        qWarning() << "OIIO Error:" << OIIO::geterror().c_str();
      }
    }
  }

  return nullptr;
}

NodeInput *VideoRenderBackend::GetDependentInput()
{
  return viewer_node()->texture_input();
}

bool VideoRenderBackend::CanRender()
{
  return params_.is_valid();
}

TimeRange VideoRenderBackend::PopNextFrameFromQueue()
{
  // Try to find the frame that's closest to the last time requested (the playhead)

  // Set up playhead frame range to see if the queue contains this frame precisely
  TimeRange test_range(last_time_requested_, last_time_requested_ + params_.time_base());

  // Use this variable to find the closest frame in the range
  rational closest_time = -1;

  for (int i=0;i<cache_queue_.size();i++) {
    const TimeRange& range_here = cache_queue_.at(i);

    if (range_here.OverlapsWith(test_range, false, false)) {
      closest_time = -1;
      break;
    }

    for (int i=0;i<2;i++) {
      rational compare;

      if (i == 0) {
        compare = Timecode::snap_time_to_timebase(range_here.in(), params_.time_base());
        if (compare > range_here.in()) {
          compare -= params_.time_base();
        }
      } else {
        compare = Timecode::snap_time_to_timebase(range_here.out(), params_.time_base());
        if (compare >= range_here.out()) {
          compare -= params_.time_base();
        }
      }

      if (closest_time < 0
          || qAbs(compare - last_time_requested_) < qAbs(closest_time - last_time_requested_)) {
        closest_time = compare;
      }
    }
  }

  TimeRange frame_range;

  if (closest_time == -1) {
    frame_range = test_range;
  } else {
    frame_range = TimeRange(closest_time, closest_time + params_.time_base());
  }

  // Remove this particular frame from the queue
  cache_queue_.RemoveTimeRange(frame_range);

  // Remove this particular frame from missing frames
  invalidated_.RemoveTimeRange(frame_range);

  // Return the snapped frame
  return TimeRange(frame_range.in(), frame_range.in());
}

void VideoRenderBackend::ThreadCompletedFrame(NodeDependency path, qint64 job_time, QByteArray hash, QVariant value)
{
  if (!only_signal_last_frame_requested_ || last_time_requested_ == path.in() || frame_cache_.TimeToHash(last_time_requested_) == hash) {
    EmitCachedFrameReady(path.in(), value, job_time);
  }

  if (!(operating_mode_ & VideoRenderWorker::kDownloadOnly)) {
    // If we're not downloading, the worker is done here
    SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

    CacheNext();
  }
}

void VideoRenderBackend::ThreadCompletedDownload(NodeDependency dep, qint64 job_time, QByteArray hash, bool texture_existed)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  SetFrameHash(dep, hash, job_time);

  // Register frame with the disk manager
  if (texture_existed && operating_mode_ & VideoRenderWorker::kDownloadOnly) {
    DiskManager::instance()->CreatedFile(frame_cache()->CachePathName(hash, params_.format()), hash);
  }

  QList<rational> hashes_with_time = frame_cache()->FramesWithHash(hash);

  foreach (const rational& t, hashes_with_time) {
    emit CachedTimeReady(t, job_time);
  }

  // Queue up a new frame for this worker
  CacheNext();
}

void VideoRenderBackend::ThreadSkippedFrame(NodeDependency dep, qint64 job_time, QByteArray hash)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  if (SetFrameHash(dep, hash, job_time)
      && frame_cache_.HasHash(hash, params_.format())) {
    emit CachedTimeReady(dep.in(), job_time);
  }

  // Queue up a new frame for this worker
  CacheNext();
}

void VideoRenderBackend::ThreadHashAlreadyExists(NodeDependency dep, qint64 job_time, QByteArray hash)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  if (SetFrameHash(dep, hash, job_time)) {
    emit CachedTimeReady(dep.in(), job_time);
  }

  // Queue up a new frame for this worker
  CacheNext();
}

void VideoRenderBackend::TruncateFrameCacheLength(const rational &length)
{
  // Remove frames after this time code if it's changed
  frame_cache_.Truncate(length);

  invalidated_.RemoveTimeRange(TimeRange(length, RATIONAL_MAX));

  // If the playhead is past the length, update the viewer to a null texture because it won't be cached through the
  // queue, but will now be a null texture
  if (last_time_requested_ >= length) {
    emit CachedFrameReady(last_time_requested_, QVariant(), QDateTime::currentMSecsSinceEpoch());
  }

  // Adjust queue for new invalidated range
  Requeue();
}

void VideoRenderBackend::FrameRemovedFromDiskCache(const QByteArray &hash)
{
  QList<rational> deleted_frames = frame_cache()->FramesWithHash(hash);

  foreach (const rational& frame, deleted_frames) {
    TimeRange invalidated(frame, frame+params_.time_base());

    invalidated_.InsertTimeRange(invalidated);

    emit RangeInvalidated(invalidated);
  }
}

bool VideoRenderBackend::TimeIsQueued(const TimeRange &time) const
{
  return cache_queue_.ContainsTimeRange(time, true, false);
}

bool VideoRenderBackend::JobIsCurrent(const NodeDependency &dep, const qint64& job_time) const
{
  return (render_job_info_.value(dep.range()) == job_time && !TimeIsQueued(dep.range()));
}

bool VideoRenderBackend::SetFrameHash(const NodeDependency &dep, const QByteArray &hash, const qint64& job_time)
{
  if (JobIsCurrent(dep, job_time)) {
    frame_cache_.SetHash(dep.in(), hash);
    render_job_info_.remove(dep.range());

    return true;
  }

  return false;
}

void VideoRenderBackend::Requeue()
{
  cache_queue_.clear();

  // Reset queue around the last time requested
  TimeRange queueable_range(last_time_requested_ - Config::Current()["DiskCacheBehind"].value<rational>(),
                            last_time_requested_ + Config::Current()["DiskCacheAhead"].value<rational>());

  cache_queue_ = invalidated_.Intersects(queueable_range);

  CacheNext();
}

void VideoRenderBackend::ResizeCacheLoadBuffer()
{
  cache_frame_load_buffer_.resize(PixelService::GetBufferSize(params_.format(), params_.effective_width(), params_.effective_height()));
}
