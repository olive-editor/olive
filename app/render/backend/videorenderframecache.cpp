#include "videorenderframecache.h"

#include <QDir>
#include <QFileInfo>

#include "common/filefunctions.h"

VideoRenderFrameCache::VideoRenderFrameCache()
{

}

void VideoRenderFrameCache::Clear()
{
  time_hash_map_.clear();

  currently_caching_lock_.lock();
  currently_caching_list_.clear();
  currently_caching_lock_.unlock();

  cache_id_.clear();
}

bool VideoRenderFrameCache::HasHash(const QByteArray &hash, const PixelFormat::Format& format)
{
  return QFileInfo::exists(CachePathName(hash, format)) && !IsCaching(hash);
}

bool VideoRenderFrameCache::IsCaching(const QByteArray &hash)
{
  currently_caching_lock_.lock();

  bool is_caching = currently_caching_list_.contains(hash);

  currently_caching_lock_.unlock();

  return is_caching;
}

bool VideoRenderFrameCache::TryCache(const rational& time, const QByteArray &hash)
{
  currently_caching_lock_.lock();

  bool is_caching = currently_caching_list_.contains(hash);

  if (!is_caching) {
    currently_caching_list_.append(hash);
  }

  currently_caching_lock_.unlock();

  return !is_caching;
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

void VideoRenderFrameCache::RemoveHash(const rational &time, const QByteArray &hash)
{
  time_hash_map_.remove(time);
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
  currently_caching_lock_.lock();
  currently_caching_list_.removeOne(hash);
  currently_caching_lock_.unlock();
}

QList<rational> VideoRenderFrameCache::FramesWithHash(const QByteArray &hash)
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

const QMap<rational, QByteArray> &VideoRenderFrameCache::time_hash_map() const
{
  return time_hash_map_;
}

QString VideoRenderFrameCache::CachePathName(const QByteArray &hash, const PixelFormat::Format& pix_fmt) const
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation());
  this_cache_dir.mkpath(".");

  QString ext;

  if (pix_fmt == PixelFormat::PIX_FMT_RGBA8) {
    // For some reason, 8-bit EXRs are extremely slow to load, so we use TIFF instead.
    ext = QStringLiteral("tiff");
  } else {
    ext = QStringLiteral("exr");
  }

  QString filename = QStringLiteral("%1.%2").arg(QString(hash.toHex()), ext);

  return this_cache_dir.filePath(filename);
}
