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

#ifndef VIDEORENDERFRAMECACHE_H
#define VIDEORENDERFRAMECACHE_H

#include <QMutex>

#include "common/rational.h"
#include "common/timerange.h"
#include "render/pixelformat.h"
#include "render/playbackcache.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class FrameHashCache : public PlaybackCache
{
  Q_OBJECT
public:
  FrameHashCache() = default;

  QByteArray GetHash(const rational& time) const;

  void SetHash(const rational& time, const QByteArray& hash);

  void SetTimebase(const rational& tb);

  /**
   * @brief Returns a list of frames that use a particular hash
   */
  QList<rational> GetFramesWithHash(const QByteArray& hash) const;

  /**
   * @brief Same as FramesWithHash() but also removes these frames from the map
   */
  QList<rational> TakeFramesWithHash(const QByteArray& hash);

  const QMap<rational, QByteArray>& time_hash_map() const;

  /**
   * @brief Return the path of the cached image at this time
   */
  static QString CachePathName(const QByteArray &hash, const PixelFormat::Format& pix_fmt);

  static bool SaveCacheFrame(const QString& filename, char *data, const VideoRenderingParams &vparam);
  static void SaveCacheFrame(const QByteArray& hash, char *data, const VideoRenderingParams &vparam);

  static QString GetFormatExtension(const PixelFormat::Format& f);

protected:
  virtual void LengthChangedEvent(const rational& old, const rational& newlen) override;

  virtual void InvalidateEvent(const TimeRange& range) override;

private:
  QMap<rational, QByteArray> time_hash_map_;

  rational timebase_;

};

OLIVE_NAMESPACE_EXIT

#endif // VIDEORENDERFRAMECACHE_H
