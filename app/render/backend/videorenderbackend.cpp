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
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QtMath>
#include <QThread>

#include "common/filefunctions.h"
#include "opengl/functions.h"
#include "render/pixelservice.h"

VideoRenderBackend::VideoRenderBackend(QObject *parent) :
  RenderBackend(parent),
  caching_(false),
  started_(false)
{
  // FIXME: Cache name should actually be the name of the sequence
  SetCacheName("Test");
}

VideoRenderBackend::~VideoRenderBackend()
{
  Close();
}

void VideoRenderBackend::SetCacheName(const QString &s)
{
  cache_name_ = s;
  cache_time_ = QDateTime::currentMSecsSinceEpoch();

  GenerateCacheIDInternal();
}

void VideoRenderBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  if (!params_.is_valid()) {
    return;
  }

  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(viewer_node()->Length(), end_range);

  qDebug() << "Cache invalidated between"
           << start_range_adj.toDouble()
           << "and"
           << end_range_adj.toDouble();

  // Snap start_range to timebase
  double start_range_dbl = start_range_adj.toDouble();
  double start_range_numf = start_range_dbl * static_cast<double>(params_.time_base().denominator());
  int64_t start_range_numround = qFloor(start_range_numf/static_cast<double>(params_.time_base().numerator())) * params_.time_base().numerator();
  rational true_start_range(start_range_numround, params_.time_base().denominator());

  for (rational r=true_start_range;r<=end_range_adj;r+=params_.time_base()) {
    // Try to order the queue from closest to the playhead to furthest
    rational last_time = last_time_requested_;

    rational diff = r - last_time;

    if (diff < 0) {
      // FIXME: Hardcoded number
      // If the number is before the playhead, we still prioritize its closeness but not nearly as much (5:1 in this
      // example)
      diff = qAbs(diff) * 5;
    }

    bool contains = false;
    bool added = false;
    QLinkedList<rational>::iterator insert_iterator;

    for (QLinkedList<rational>::iterator i = cache_queue_.begin();i != cache_queue_.end();i++) {
      rational compare = *i;

      if (!added) {
        rational compare_diff = compare - last_time;

        if (compare_diff > diff) {
          insert_iterator = i;
          added = true;
        }
      }

      if (compare == r) {
        contains = true;
        break;
      }
    }

    if (!contains) {
      if (added) {
        cache_queue_.insert(insert_iterator, r);
      } else {
        cache_queue_.append(r);
      }
    }
  }

  CacheNext();
}

void VideoRenderBackend::ViewerNodeChangedEvent(ViewerOutput *node)
{
  if (node != nullptr) {
    // FIXME: Hardcoded format, mode, and divider
    SetParameters(VideoRenderingParams(node->video_params(), olive::PIX_FMT_RGBA16F, olive::kOffline, 2));
  }
}

const QVector<QThread *> &VideoRenderBackend::threads()
{
  return threads_;
}

const VideoRenderingParams &VideoRenderBackend::params() const
{
  return params_;
}

void VideoRenderBackend::SetParameters(const VideoRenderingParams& params)
{
  // Since we're changing parameters, all the existing threads are invalid and must be removed. They will start again
  // next time this Node has to process anything.
  Close();

  // Set new parameters
  params_ = params;

  // Regenerate the cache ID
  GenerateCacheIDInternal();
}

bool VideoRenderBackend::Init()
{
  if (started_) {
    return true;
  }

  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    QThread* thread = new QThread(this);
    threads_.replace(i, thread);

    // We use low priority to keep the app responsive at all times (GUI thread should always prioritize over this one)
    thread->start(QThread::LowPriority);
  }

  cache_frame_load_buffer_.resize(PixelService::GetBufferSize(params_.format(), params_.effective_width(), params_.effective_height()));

  started_ = true;

  return true;
}

void VideoRenderBackend::Close()
{
  if (!started_) {
    return;
  }

  started_ = false;

  foreach (QThread* thread, threads_) {
    thread->quit();
  }
  threads_.clear();

  cache_frame_load_buffer_.clear();
}

void VideoRenderBackend::GenerateCacheIDInternal()
{
  if (cache_name_.isEmpty() || !params_.is_valid()) {
    return;
  }

  // Generate an ID that is more or less guaranteed to be unique to this Sequence
  QCryptographicHash hash(QCryptographicHash::Sha1);
  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());
  hash.addData(QString::number(params_.width()).toUtf8());
  hash.addData(QString::number(params_.height()).toUtf8());
  hash.addData(QString::number(params_.format()).toUtf8());
  hash.addData(QString::number(params_.divider()).toUtf8());

  QByteArray bytes = hash.result();
  cache_id_ = bytes.toHex();
}

void VideoRenderBackend::CacheNext()
{
  if (cache_queue_.isEmpty() || viewer_node() == nullptr || caching_) {
    return;
  }

  // Make sure cache has started
  Init();

  rational cache_frame = cache_queue_.takeFirst();

  qDebug() << "Caching" << cache_frame.toDouble();

  GenerateFrame(cache_frame);

  caching_ = true;
}

QString VideoRenderBackend::CachePathName(const QByteArray &hash)
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1.exr").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}

void VideoRenderBackend::DeferMap(const rational &time, const QByteArray &hash)
{
  deferred_maps_.append({time, hash});
}

bool VideoRenderBackend::HasHash(const QByteArray &hash)
{
  return QFileInfo::exists(CachePathName(hash));
}

bool VideoRenderBackend::IsCaching(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();

  bool is_caching = cache_hash_list_.contains(hash);

  cache_hash_list_mutex_.unlock();

  return is_caching;
}

bool VideoRenderBackend::TryCache(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();

  bool is_caching = cache_hash_list_.contains(hash);

  if (!is_caching) {
    cache_hash_list_.append(hash);
  }

  cache_hash_list_mutex_.unlock();

  return !is_caching;
}

const char *VideoRenderBackend::GetCachedFrame(const rational &time)
{
  last_time_requested_ = time;

  if (viewer_node() == nullptr) {
    // Nothing is connected - nothing to show or render
    return nullptr;
  }

  if (cache_id_.isEmpty()) {
    qWarning() << "No cache ID";
    return nullptr;
  }

  if (!params_.is_valid()) {
    qWarning() << "Invalid parameters";
    return nullptr;
  }

  // Find frame in map
  if (time_hash_map_.contains(time)) {
    QString fn = CachePathName(time_hash_map_[time]);

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

bool VideoRenderBackend::IsStarted()
{
  return started_;
}
