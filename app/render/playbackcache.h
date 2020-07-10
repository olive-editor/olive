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

#ifndef PLAYBACKCACHE_H
#define PLAYBACKCACHE_H

#include <QMutex>
#include <QObject>

#include "common/timerange.h"

OLIVE_NAMESPACE_ENTER

class PlaybackCache : public QObject
{
  Q_OBJECT
public:
  PlaybackCache(QObject* parent = nullptr) :
    QObject(parent)
  {
  }

  void Invalidate(const TimeRange& r);

  void InvalidateAll();

  const rational& GetLength()
  {
    QMutexLocker locker(lock());

    return NoLockGetLength();
  }

  void SetLength(const rational& r);

  bool IsFullyValidated()
  {
    QMutexLocker locker(lock());

    return invalidated_.isEmpty();
  }

  void Shift(const rational& from, const rational& to);

  const TimeRangeList& GetInvalidatedRanges()
  {
    QMutexLocker locker(lock());

    return NoLockGetInvalidatedRanges();
  }

  bool HasInvalidatedRanges()
  {
    QMutexLocker locker(lock());

    return !invalidated_.isEmpty();
  }

signals:
  void Invalidated(const OLIVE_NAMESPACE::TimeRange& r);

  void Validated(const OLIVE_NAMESPACE::TimeRange& r);

  void Shifted(const OLIVE_NAMESPACE::rational& from, const OLIVE_NAMESPACE::rational& to);

  void LengthChanged(const OLIVE_NAMESPACE::rational& r);

protected:
  void NoLockInvalidate(const TimeRange& r);

  void NoLockValidate(const TimeRange& r);

  const rational& NoLockGetLength() const
  {
    return length_;
  }

  const TimeRangeList& NoLockGetInvalidatedRanges()
  {
    return invalidated_;
  }

  virtual void LengthChangedEvent(const rational& old, const rational& newlen);

  virtual void InvalidateEvent(const TimeRange& range);

  virtual void ShiftEvent(const rational& from, const rational& to);

  QMutex* lock()
  {
    return &lock_;
  }

  struct JobIdentifier {
    TimeRange range;
    qint64 job_time;
  };

  QList<JobIdentifier> jobs_;

private:
  void RemoveRangeFromJobs(const TimeRange& remove);

  QMutex lock_;

  TimeRangeList invalidated_;

  rational length_;

};

OLIVE_NAMESPACE_EXIT

#endif // PLAYBACKCACHE_H
