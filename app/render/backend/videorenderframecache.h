#ifndef VIDEORENDERFRAMECACHE_H
#define VIDEORENDERFRAMECACHE_H

#include <QMutex>

#include "common/rational.h"
#include "render/pixelformat.h"

class VideoRenderFrameCache
{
public:
  VideoRenderFrameCache();

  void Clear();

  /**
   * @brief Return whether a frame with this hash already exists
   */
  bool HasHash(const QByteArray& hash, const PixelFormat::Format &format);

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
  QString CachePathName(const QByteArray &hash, const PixelFormat::Format& pix_fmt) const;

  void SetCacheID(const QString& id);

  QByteArray TimeToHash(const rational& time) const;

  void SetHash(const rational& time, const QByteArray& hash);

  void Truncate(const rational& time);

  void RemoveHashFromCurrentlyCaching(const QByteArray& hash);

  /**
   * @brief Returns a list of frames that use a particular hash
   */
  QList<rational> FramesWithHash(const QByteArray& hash) const;

  /**
   * @brief Same as FramesWithHash() but also removes these frames from the map
   */
  QList<rational> TakeFramesWithHash(const QByteArray& hash);

  const QMap<rational, QByteArray>& time_hash_map() const;

private:
  QMap<rational, QByteArray> time_hash_map_;

  QMutex currently_caching_lock_;
  QVector<QByteArray> currently_caching_list_;

  QString cache_id_;
};

#endif // VIDEORENDERFRAMECACHE_H
