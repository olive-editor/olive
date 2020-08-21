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
  FrameHashCache(QObject* parent = nullptr);

  QByteArray GetHash(const rational& time);

  void SetTimebase(const rational& tb);

  void ValidateFramesWithHash(const QByteArray& hash);

  /**
   * @brief Returns a list of frames that use a particular hash
   */
  QList<rational> GetFramesWithHash(const QByteArray& hash);

  /**
   * @brief Same as FramesWithHash() but also removes these frames from the map
   */
  QList<rational> TakeFramesWithHash(const QByteArray& hash);

  QMap<rational, QByteArray> time_hash_map();

  /**
   * @brief Return the path of the cached image at this time
   */
  QString CachePathName(const QByteArray &hash) const;

  bool SaveCacheFrame(const QString& filename, char *data, const VideoParams &vparam, int linesize_bytes) const;
  bool SaveCacheFrame(const QByteArray& hash, char *data, const VideoParams &vparam, int linesize_bytes) const;
  bool SaveCacheFrame(const QByteArray& hash, FramePtr frame) const;
  FramePtr LoadCacheFrame(const QByteArray& hash) const;
  FramePtr LoadCacheFrame(const QString& fn) const;

  static QString GetFormatExtension();

  static QVector<rational> GetFrameListFromTimeRange(TimeRangeList range_list, const rational& timebase);
  QVector<rational> GetFrameListFromTimeRange(const TimeRangeList &range);
  QVector<rational> GetInvalidatedFrames();
  QVector<rational> GetInvalidatedFrames(const TimeRange& intersecting);

public slots:
  void SetHash(const OLIVE_NAMESPACE::rational& time, const QByteArray& hash, const qint64 &job_time, bool frame_exists);

protected:
  virtual void LengthChangedEvent(const rational& old, const rational& newlen) override;

  virtual void ShiftEvent(const rational& from, const rational& to) override;

  virtual void InvalidateEvent(const TimeRange& range) override;

private:
  QMap<rational, QByteArray> time_hash_map_;

  rational timebase_;

private slots:
  void HashDeleted(const QString &s, const QByteArray& hash);

  void ProjectInvalidated(Project* p);

};

OLIVE_NAMESPACE_EXIT

#endif // VIDEORENDERFRAMECACHE_H
