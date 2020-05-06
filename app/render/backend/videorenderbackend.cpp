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
#include "render/pixelformat.h"
#include "videorenderworker.h"

OLIVE_NAMESPACE_ENTER

VideoRenderBackend::VideoRenderBackend(QObject *parent) :
  RenderBackend(parent),
  operating_mode_(VideoRenderWorker::kHashRenderCache),
  only_signal_last_frame_requested_(true),
  limit_caching_(true),
  pop_toggle_(false)
{
  connect(DiskManager::instance(), &DiskManager::DeletedFrame, this, &VideoRenderBackend::FrameRemovedFromDiskCache);
}

void VideoRenderBackend::ConnectViewer(ViewerOutput *node)
{
  connect(node, &ViewerOutput::VideoChangedBetween, this, &VideoRenderBackend::InvalidateCache);
  connect(node, &ViewerOutput::LengthChanged, this, &VideoRenderBackend::TruncateFrameCacheLength);
}

void VideoRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, &ViewerOutput::VideoChangedBetween, this, &VideoRenderBackend::InvalidateCache);
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
  CancelQueue();

  operating_mode_ = mode;

  foreach (RenderWorker* worker, processors_) {
    static_cast<VideoRenderWorker*>(worker)->SetOperatingMode(operating_mode_);
  }
}

void VideoRenderBackend::SetFrameGenerationParams(int width, int height, const QMatrix4x4 &matrix)
{
  foreach (RenderWorker* worker, processors_) {
    static_cast<VideoRenderWorker*>(worker)->SetFrameGenerationParams(width, height, matrix);
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

void VideoRenderBackend::SetLimitCaching(bool limit)
{
  limit_caching_ = limit;
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

  connect(video_processor, &VideoRenderWorker::HashAlreadyBeingCached, this, &VideoRenderBackend::ThreadSkippedFrame, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::CompletedDownload, this, &VideoRenderBackend::ThreadCompletedDownload, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::HashAlreadyExists, this, &VideoRenderBackend::ThreadHashAlreadyExists, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::GeneratedFrame, this, &VideoRenderBackend::GeneratedFrame, Qt::QueuedConnection);
  connect(video_processor, &VideoRenderWorker::GeneratedFrame, this, &VideoRenderBackend::ThreadGeneratedFrame, Qt::QueuedConnection);
}

void VideoRenderBackend::InvalidateCacheInternal(const rational &start_range, const rational &end_range, bool only_visible)
{
  TimeRange invalidated(start_range, end_range);

  invalidated_.InsertTimeRange(invalidated);

  emit RangeInvalidated(invalidated);

  if (only_visible) {

    // We're only caching this frame, and for maximum responsiveness, should cancel the rest of the
    // queue
    cache_queue_.clear();
    cache_queue_.InsertTimeRange(TimeRange(start_range, end_range));

    CacheNext();

  } else {

    // Rework the queue
    Requeue();

  }
}

VideoRenderFrameCache *VideoRenderBackend::frame_cache()
{
  return &frame_cache_;
}

QString VideoRenderBackend::GetCachedFrame(const rational &time)
{
  UpdateLastRequestedTime(time);

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

  // Find frame in map
  QByteArray frame_hash = frame_cache_.TimeToHash(time);

  if (!frame_hash.isEmpty()) {
    DiskManager::instance()->Accessed(frame_hash);

    return frame_cache_.CachePathName(frame_hash, params_.format());
  }

  return QString();
}

void VideoRenderBackend::UpdateLastRequestedTime(const rational &time)
{
  if (last_time_requested_ != time) {
    last_time_requested_ = time;

    Requeue();
  }
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
  rational earliest_allowed_time = (pop_toggle_) ? 0 : last_time_requested_;
  pop_toggle_ = !pop_toggle_;

  // Set up playhead frame range to see if the queue contains this frame precisely
  TimeRange test_range(earliest_allowed_time, earliest_allowed_time + params_.time_base());

  // Use this variable to find the closest frame in the range
  rational closest_time = RATIONAL_MAX;

  foreach (const TimeRange& range_here, cache_queue_) {
    if (range_here.OverlapsWith(test_range, false, false)) {
      closest_time = RATIONAL_MAX;
      break;
    }

    if (range_here.in() >= earliest_allowed_time) {
      rational frame_here = Timecode::snap_time_to_timebase(range_here.in(), params_.time_base());

      if (frame_here > range_here.in()) {
        frame_here = qMax(rational(), frame_here - params_.time_base());
      }

      closest_time = qMin(closest_time, frame_here);
    }
  }

  TimeRange frame_range;

  if (closest_time == RATIONAL_MAX) {
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

void VideoRenderBackend::ThreadCompletedDownload(NodeDependency dep, qint64 job_time, QByteArray hash)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  SetFrameHash(dep, hash, job_time);

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

void VideoRenderBackend::ThreadGeneratedFrame()
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

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
    emit CachedTimeReady(last_time_requested_, QDateTime::currentMSecsSinceEpoch());
  }

  // Adjust queue for new invalidated range
  Requeue();
}

void VideoRenderBackend::FrameRemovedFromDiskCache(const QByteArray &hash)
{
  QList<rational> deleted_frames = frame_cache()->TakeFramesWithHash(hash);

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
  if (limit_caching_) {

    // Reset queue around the last time requested
    TimeRange queueable_range(last_time_requested_ - Config::Current()["DiskCacheBehind"].value<rational>(),
                              last_time_requested_ + Config::Current()["DiskCacheAhead"].value<rational>());

    cache_queue_ = invalidated_.Intersects(queueable_range);

  } else {

    cache_queue_ = invalidated_;

  }

  CacheNext();
}

OLIVE_NAMESPACE_EXIT
