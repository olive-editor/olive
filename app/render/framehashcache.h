/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QUuid>

#include "common/rational.h"
#include "common/timecodefunctions.h"
#include "common/timerange.h"
#include "codec/frame.h"
#include "render/playbackcache.h"
#include "render/videoparams.h"

namespace olive {

class FrameHashCache : public PlaybackCache
{
  Q_OBJECT
public:
  FrameHashCache(QObject* parent = nullptr);

  const QUuid &GetUuid() const { return uuid_; }
  void SetUuid(const QUuid &u) { uuid_ = u; }

  const rational &GetTimebase() const { return timebase_; }

  void SetTimebase(const rational& tb);

  void ValidateTimestamp(const int64_t &ts);
  void ValidateTime(const rational &time);

  /**
   * @brief Return the path of the cached image at this time
   */
  QString CachePathName(const int64_t &time) const;
  QString CachePathName(const rational &time) const
  {
    return CachePathName(ToTimestamp(time, Timecode::kFloor));
  }

  static QString CachePathName(const QString& cache_path, const QUuid &cache_id, const int64_t &time);

  static bool SaveCacheFrame(const QString& filename, FramePtr frame);
  bool SaveCacheFrame(const int64_t &time, FramePtr frame) const;
  static bool SaveCacheFrame(const QString& cache_path, const QUuid &uuid, const int64_t &time, FramePtr frame);
  static FramePtr LoadCacheFrame(const QString& cache_path, const QUuid &uuid, const int64_t &time);
  FramePtr LoadCacheFrame(const int64_t &time) const;
  static FramePtr LoadCacheFrame(const QString& fn);

private:
  rational ToTime(const int64_t &ts) const;
  int64_t ToTimestamp(const rational &ts, Timecode::Rounding rounding = Timecode::kRound) const;

  rational timebase_;

  QUuid uuid_;

  static QMutex currently_saving_frames_mutex_;
  static QMap<QString, FramePtr> currently_saving_frames_;
  static const QString kCacheFormatExtension;

private slots:
  void HashDeleted(const QString &path, const QString &filename);

  void ProjectInvalidated(Project* p);

};

}

#endif // VIDEORENDERFRAMECACHE_H
