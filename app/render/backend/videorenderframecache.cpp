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

bool VideoRenderFrameCache::HasHash(const QByteArray &hash)
{
  return QFileInfo::exists(CachePathName(hash)) && !IsCaching(hash);
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

QString VideoRenderFrameCache::CachePathName(const QByteArray &hash) const
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1.exr").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}
