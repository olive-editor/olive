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
#include "render/pixelservice.h"
#include "videorenderworker.h"

VideoRenderBackend::VideoRenderBackend(QObject *parent) :
  RenderBackend(parent),
  export_mode_(false),
  last_download_thread_(0)
{
}

void VideoRenderBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  if (!params_.is_valid()) {
    return;
  }

  RenderBackend::InvalidateCache(start_range, end_range);

  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(GetSequenceLength(), end_range);

  qDebug() << "Cache invalidated between"
           << start_range_adj.toDouble()
           << "and"
           << end_range_adj.toDouble();

  // Snap start_range to timebase
  int64_t timestamp = olive::time_to_timestamp(start_range_adj, params_.time_base());
  rational true_start = olive::timestamp_to_time(timestamp, params_.time_base());

  for (rational r=true_start;r<end_range_adj;r+=params_.time_base()) {
    // Try to order the queue from closest to the playhead to furthest
    rational last_time = last_time_requested_;

    rational diff = r - last_time;

    if (diff < 0) {
      // FIXME: Hardcoded number
      // If the number is before the playhead, we still prioritize its closeness but not nearly as much (5:1 in this
      // example)
      diff = qAbs(diff) * 5;
    }

    bool added = false;

    TimeRange new_range(r, r);

    for (int i=0;i<cache_queue_.size();i++) {
      rational compare = cache_queue_.at(i).in();
      rational compare_diff = compare - last_time;

      if (compare == r) {
        added = true;
        break;
      }

      if (compare_diff > diff)  {
        cache_queue_.insert(i, new_range);
        added = true;
        break;
      }
    }

    if (!added) {
      cache_queue_.append(new_range);
    }
  }

  // Remove frames after this time code if it's changed
  frame_cache_.Truncate(GetSequenceLength());

  // Queue value update
  QueueValueUpdate();

  CacheNext();
}

bool VideoRenderBackend::InitInternal()
{
  cache_frame_load_buffer_.resize(PixelService::GetBufferSize(params_.format(), params_.effective_width(), params_.effective_height()));
  return true;
}

void VideoRenderBackend::CloseInternal()
{
  cache_frame_load_buffer_.clear();
}

void VideoRenderBackend::ConnectViewer(ViewerOutput *node)
{
  connect(node, SIGNAL(VideoChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  connect(node, SIGNAL(VideoGraphChanged()), this, SLOT(QueueRecompile()));
}

void VideoRenderBackend::DisconnectViewer(ViewerOutput *node)
{
  disconnect(node, SIGNAL(VideoChangedBetween(const rational&, const rational&)), this, SLOT(InvalidateCache(const rational&, const rational&)));
  disconnect(node, SIGNAL(VideoGraphChanged()), this, SLOT(QueueRecompile()));
}

const VideoRenderingParams &VideoRenderBackend::params() const
{
  return params_;
}

void VideoRenderBackend::SetParameters(const VideoRenderingParams& params)
{
  // Set new parameters
  params_ = params;

  // Set params on all processors
  // FIXME: Undefined behavior if the processors are currently working, this may need to be delayed like the
  //        recompile signal
  foreach (RenderWorker* worker, processors_) {
    static_cast<VideoRenderWorker*>(worker)->SetParameters(params_);
  }

  // Regenerate the cache ID
  RegenerateCacheID();
}

void VideoRenderBackend::SetExportMode(bool enabled)
{
  export_mode_ = enabled;
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
  connect(processor, SIGNAL(CompletedFrame(NodeDependency, QByteArray, NodeValueTable)), this, SLOT(ThreadCompletedFrame(NodeDependency, QByteArray, NodeValueTable)));
  connect(processor, SIGNAL(HashAlreadyBeingCached(NodeDependency, QByteArray)), this, SLOT(ThreadSkippedFrame(NodeDependency, QByteArray)));
  connect(processor, SIGNAL(CompletedDownload(NodeDependency, QByteArray)), this, SLOT(ThreadCompletedDownload(NodeDependency, QByteArray)));
  connect(processor, SIGNAL(HashAlreadyExists(NodeDependency, QByteArray)), this, SLOT(ThreadHashAlreadyExists(NodeDependency, QByteArray)));
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

  // Find frame in map
  QByteArray frame_hash = frame_cache_.TimeToHash(time);

  if (!frame_hash.isEmpty()) {
    QString fn = frame_cache_.CachePathName(frame_hash);

    if (QFileInfo::exists(fn)) {
      auto in = OIIO::ImageInput::open(fn.toStdString());

      if (in) {
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

void VideoRenderBackend::ThreadCompletedFrame(NodeDependency path, QByteArray hash, NodeValueTable table)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  QVariant texture = table.Get(NodeParam::kTexture);

  // Check if this frame has changed once again, in which case we may not want to draw it (it'll look jittery to the user)
  if (!TimeIsQueued(TimeRange(path.in(), path.in()))) {
    EmitCachedFrameReady(path.in(), texture);

    if (export_mode_) {
      QList<rational> times_with_this_hash = frame_cache()->DeferredMapsWithHash(hash);
      foreach (const rational& t, times_with_this_hash) {
        EmitCachedFrameReady(t, texture);
      }
    }
  }

  if (!export_mode_) {
    if (texture.isNull()) {
      // No frame received, we set hash to an empty
      frame_cache()->RemoveHash(path.in(), hash);
    } else {
      // Received a texture, let's download it
      QString cache_fn = frame_cache()->CachePathName(hash);

      // Find an available worker to download this texture
      QMetaObject::invokeMethod(processors_.at(last_download_thread_%processors_.size()),
                                "Download",
                                Q_ARG(NodeDependency, path),
                                Q_ARG(QByteArray, hash),
                                Q_ARG(QVariant, texture),
                                Q_ARG(QString, cache_fn));

      last_download_thread_++;
    }
  }

  // Queue up a new frame for this worker
  CacheNext();
}

void VideoRenderBackend::ThreadCompletedDownload(NodeDependency dep, QByteArray hash)
{
  frame_cache()->SetHash(dep.in(), hash);
  emit CachedTimeReady(dep.in());

  // Emit for each frame that has this hash (some may have been added in ThreadSkippedFrame)
  QList<rational> times_with_this_hash = frame_cache()->DeferredMapsWithHash(hash);
  foreach (const rational& t, times_with_this_hash) {
    frame_cache()->SetHash(t, hash);
    emit CachedTimeReady(t);
  }
}

void VideoRenderBackend::ThreadSkippedFrame(NodeDependency dep, QByteArray hash)
{
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  // Queue up a new frame for this worker
  CacheNext();
}

void VideoRenderBackend::ThreadHashAlreadyExists(NodeDependency dep, QByteArray hash)
{
  ThreadCompletedDownload(dep, hash);
  SetWorkerBusyState(static_cast<RenderWorker*>(sender()), false);

  // Queue up a new frame for this worker
  CacheNext();
}

bool VideoRenderBackend::TimeIsQueued(const TimeRange &time)
{
  return cache_queue_.contains(time);
}
