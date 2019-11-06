#ifndef VIDEORENDERFRAMECACHE_H
#define VIDEORENDERFRAMECACHE_H

#include <QMutex>

#include "common/rational.h"

class VideoRenderFrameCache
{
public:
  VideoRenderFrameCache();

  /**
   * @brief Return whether a frame with this hash already exists
   */
  bool HasHash(const QByteArray& hash);

  /**
   * @brief Return whether a frame is currently being cached
   */
  bool IsCaching(const QByteArray& hash);

  /**
   * @brief Check if a frame is currently being cached, and if not reserve it
   */
  bool TryCache(const QByteArray& hash);

  /**
   * @brief Return the path of the cached image at this time
   */
  QString CachePathName(const QByteArray &hash);

  void SetCacheID(const QString& id);

  QByteArray TimeToHash(const rational& time);

private:
  QMap<rational, QByteArray> time_hash_map_;

  QMutex cache_hash_list_mutex_;
  QVector<QByteArray> cache_hash_list_;

  QString cache_id_;
};

#endif // VIDEORENDERFRAMECACHE_H
