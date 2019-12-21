#include "videorenderframecache.h"

#include <QDir>
#include <QFileInfo>

#include "common/filefunctions.h"

VideoRenderFrameCache::VideoRenderFrameCache()
{

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

  if (is_caching) {
    deferred_maps_.insert(time, hash);
  } else {
    currently_caching_list_.append(hash);
  }

  currently_caching_lock_.unlock();

  return !is_caching;
}

void VideoRenderFrameCache::SetCacheID(const QString &id)
{
  cache_id_ = id;
}

QByteArray VideoRenderFrameCache::TimeToHash(const rational &time) const
{
  return time_hash_map_.value(time);
}

void VideoRenderFrameCache::SetHash(const rational &time, const QByteArray &hash)
{
  // No longer currently caching this frame
  RemoveHashFromCurrentlyCaching(hash);

  // Insert frame into map
  time_hash_map_.insert(time, hash);
}

void VideoRenderFrameCache::RemoveHash(const rational &time, const QByteArray &hash)
{
  RemoveHashFromCurrentlyCaching(hash);

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

QList<rational> VideoRenderFrameCache::DeferredMapsWithHash(const QByteArray &hash)
{
  QList<rational> list;

  currently_caching_lock_.lock();

  QMap<rational, QByteArray>::iterator iterator = deferred_maps_.begin();

  while (iterator != deferred_maps_.end()) {
    if (iterator.value() == hash) {
      list.append(iterator.key());
      iterator = deferred_maps_.erase(iterator);
    } else {
      iterator++;
    }
  }

  currently_caching_list_.removeOne(hash);

  currently_caching_lock_.unlock();

  return list;
}

void VideoRenderFrameCache::RemoveHashFromCurrentlyCaching(const QByteArray &hash)
{
  currently_caching_lock_.lock();
  currently_caching_list_.removeOne(hash);
  currently_caching_lock_.unlock();
}

QString VideoRenderFrameCache::CachePathName(const QByteArray &hash) const
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1.exr").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}
