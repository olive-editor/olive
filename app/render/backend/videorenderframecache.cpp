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

#include "videorenderframecache.h"

#include <QDir>
#include <QFileInfo>

#include "common/filefunctions.h"

OLIVE_NAMESPACE_ENTER

void VideoRenderFrameCache::Clear()
{
  time_hash_map_.clear();

  {
    QMutexLocker locker(&currently_caching_lock_);
    currently_caching_list_.clear();
  }

  cache_id_.clear();
}

bool VideoRenderFrameCache::HasHash(const QByteArray &hash, const PixelFormat::Format& format)
{
  return QFileInfo::exists(CachePathName(hash, format)) && !IsCaching(hash);
}

bool VideoRenderFrameCache::IsCaching(const QByteArray &hash)
{
  QMutexLocker locker(&currently_caching_lock_);

  return currently_caching_list_.contains(hash);
}

bool VideoRenderFrameCache::TryCache(const QByteArray &hash)
{
  QMutexLocker locker(&currently_caching_lock_);

  if (!currently_caching_list_.contains(hash)) {
    currently_caching_list_.append(hash);
    return true;
  }

  return false;
}

void VideoRenderFrameCache::SetCacheID(const QString &id)
{
  Clear();

  cache_id_ = id;
}

QByteArray VideoRenderFrameCache::TimeToHash(const rational &time) const
{
  return time_hash_map_.value(time);
}

void VideoRenderFrameCache::SetHash(const rational &time, const QByteArray &hash)
{
  time_hash_map_.insert(time, hash);
}

void VideoRenderFrameCache::Truncate(const rational &time)
{
  QMap<rational, QByteArray>::iterator i = time_hash_map_.begin();

  while (i != time_hash_map_.end()) {
    if (i.key() >= time) {
      i = time_hash_map_.erase(i);
    } else {
      i++;
    }
  }
}

void VideoRenderFrameCache::RemoveHashFromCurrentlyCaching(const QByteArray &hash)
{
  QMutexLocker locker(&currently_caching_lock_);

  currently_caching_list_.removeOne(hash);
}

QList<rational> VideoRenderFrameCache::FramesWithHash(const QByteArray &hash) const
{
  QList<rational> times;

  QMap<rational, QByteArray>::const_iterator iterator;

  for (iterator=time_hash_map_.begin();iterator!=time_hash_map_.end();iterator++) {
    if (iterator.value() == hash) {
      times.append(iterator.key());
    }
  }

  return times;
}

QList<rational> VideoRenderFrameCache::TakeFramesWithHash(const QByteArray &hash)
{
  QList<rational> times;

  QMap<rational, QByteArray>::iterator iterator = time_hash_map_.begin();

  while (iterator != time_hash_map_.end()) {
    if (iterator.value() == hash) {
      times.append(iterator.key());

      iterator = time_hash_map_.erase(iterator);
    } else {
      iterator++;
    }
  }

  return times;
}

const QMap<rational, QByteArray> &VideoRenderFrameCache::time_hash_map() const
{
  return time_hash_map_;
}

QString VideoRenderFrameCache::CachePathName(const QByteArray& hash, const PixelFormat::Format& pix_fmt) const
{
  QString ext;

  if (pix_fmt == PixelFormat::PIX_FMT_RGB8
      || pix_fmt == PixelFormat::PIX_FMT_RGBA8
      || pix_fmt == PixelFormat::PIX_FMT_RGB16U
      || pix_fmt == PixelFormat::PIX_FMT_RGBA16U) {
    ext = QStringLiteral("jpg");
  } else {
    ext = QStringLiteral("exr");
  }

  QDir cache_dir(QDir(FileFunctions::GetMediaCacheLocation()).filePath(QString(hash.left(1).toHex())));
  cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1.%2").arg(QString(hash.mid(1).toHex()), ext);

  return cache_dir.filePath(filename);
}

OLIVE_NAMESPACE_EXIT
