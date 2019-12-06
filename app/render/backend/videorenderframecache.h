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
  QString CachePathName(const QByteArray &hash) const;

  void SetCacheID(const QString& id);

  QByteArray TimeToHash(const rational& time) const;

  void SetHash(const rational& time, const QByteArray& hash);
  void RemoveHash(const rational& time, const QByteArray &hash);

  void Truncate(const rational& time);

  QList<rational> TimesWithHash(const QByteArray& hash);

private:
  void RemoveHashFromCurrentlyCaching(const QByteArray& hash);

  QMap<rational, QByteArray> time_hash_map_;

  QMutex currently_caching_lock_;
  QVector<QByteArray> currently_caching_list_;

  QString cache_id_;
};

#endif // VIDEORENDERFRAMECACHE_H
