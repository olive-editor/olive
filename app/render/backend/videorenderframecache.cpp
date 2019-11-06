#include "videorenderframecache.h"

#include <QDir>
#include <QFileInfo>

#include "common/filefunctions.h"

VideoRenderFrameCache::VideoRenderFrameCache()
{

}

bool VideoRenderFrameCache::HasHash(const QByteArray &hash)
{
  return QFileInfo::exists(CachePathName(hash));
}

bool VideoRenderFrameCache::IsCaching(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();

  bool is_caching = cache_hash_list_.contains(hash);

  cache_hash_list_mutex_.unlock();

  return is_caching;
}

bool VideoRenderFrameCache::TryCache(const QByteArray &hash)
{
  cache_hash_list_mutex_.lock();

  bool is_caching = cache_hash_list_.contains(hash);

  if (!is_caching) {
    cache_hash_list_.append(hash);
  }

  cache_hash_list_mutex_.unlock();

  return !is_caching;
}

void VideoRenderFrameCache::SetCacheID(const QString &id)
{
  cache_id_ = id;
}

QByteArray VideoRenderFrameCache::TimeToHash(const rational &time)
{
  return time_hash_map_.value(time);
}

QString VideoRenderFrameCache::CachePathName(const QByteArray &hash)
{
  QDir this_cache_dir = QDir(GetMediaCacheLocation()).filePath(cache_id_);
  this_cache_dir.mkpath(".");

  QString filename = QStringLiteral("%1.exr").arg(QString(hash.toHex()));

  return this_cache_dir.filePath(filename);
}
