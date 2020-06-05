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

#ifndef AUDIOPLAYBACKCACHE_H
#define AUDIOPLAYBACKCACHE_H

#include "common/timerange.h"
#include "codec/samplebuffer.h"
#include "render/playbackcache.h"

OLIVE_NAMESPACE_ENTER

class AudioPlaybackCache : public PlaybackCache
{
  Q_OBJECT
public:
  AudioPlaybackCache();

  AudioRenderingParams GetParameters() {
    QMutexLocker locker(lock());
    return params_;
  }

  void SetParameters(const AudioRenderingParams& params);

  void WritePCM(const TimeRange &range, SampleBufferPtr samples, const qint64& job_time);

  void WriteSilence(const TimeRange &range);

  const QString& GetCacheFilename() const;

  QList<TimeRange> GetValidRanges(const TimeRange &range, const qint64 &job_time)
  {
    QMutexLocker locker(lock());

    return NoLockGetValidRanges(range, job_time);
  }

signals:
  void ParametersChanged();

protected:
  virtual void ShiftEvent(const rational& from, const rational& to) override;

  virtual void LengthChangedEvent(const rational& old, const rational& newlen) override;

private:
  QList<TimeRange> NoLockGetValidRanges(const TimeRange &range, const qint64 &job_time);

  QString filename_;

  AudioRenderingParams params_;

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOPLAYBACKCACHE_H
