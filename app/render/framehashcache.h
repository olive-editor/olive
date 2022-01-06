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

  QByteArray GetHash(const int64_t& time);
  QByteArray GetHash(const rational& time);

  const rational &GetTimebase() const
  {
    return timebase_;
  }

  void SetTimebase(const rational& tb);

  void ValidateFramesWithHash(const QByteArray& hash);

  /**
   * @brief Return the path of the cached image at this time
   */
  QString CachePathName(const QByteArray &hash) const;
  static QString CachePathName(const QString& cache_path, const QByteArray &hash);

  static bool SaveCacheFrame(const QString& filename, char *data, const VideoParams &vparam, int linesize_bytes);
  bool SaveCacheFrame(const QByteArray& hash, char *data, const VideoParams &vparam, int linesize_bytes) const;
  bool SaveCacheFrame(const QByteArray& hash, FramePtr frame) const;
  static bool SaveCacheFrame(const QString& cache_path, const QByteArray& hash, char *data, const VideoParams &vparam, int linesize_bytes);
  static bool SaveCacheFrame(const QString& cache_path, const QByteArray& hash, FramePtr frame);
  static FramePtr LoadCacheFrame(const QString& cache_path, const QByteArray& hash);
  FramePtr LoadCacheFrame(const QByteArray& hash) const;
  static FramePtr LoadCacheFrame(const QString& fn);

  void SetHash(const olive::rational &time, const QByteArray& hash, bool frame_exists);

protected:
  virtual void ShiftEvent(const rational& from, const rational& to) override;

  virtual void InvalidateEvent(const TimeRange& range) override;

private:
  rational ToTime(const int64_t &ts) const;
  int64_t ToTimestamp(const rational &ts, Timecode::Rounding rounding = Timecode::kRound) const;

  int64_t GetMapSize() const
  {
    return int64_t(time_hash_map_.size());
  }

  std::vector<QByteArray> time_hash_map_;
  std::map<QByteArray, std::vector<int64_t> > hash_time_map_;

  rational timebase_;

  static QMutex currently_saving_frames_mutex_;
  static QMap<QByteArray, FramePtr> currently_saving_frames_;
  static const QString kCacheFormatExtension;

private slots:
  void HashDeleted(const QString &s, const QByteArray& hash);

  void ProjectInvalidated(Project* p);

};

}

#endif // VIDEORENDERFRAMECACHE_H
