/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

class Project;

class PlaybackCache : public QObject
{
  Q_OBJECT
public:
  PlaybackCache(QObject* parent = nullptr) :
    QObject(parent)
  {
  }

  const rational& GetLength()
  {
    return length_;
  }

  bool IsFullyValidated()
  {
    return invalidated_.isEmpty();
  }

  const TimeRangeList& GetInvalidatedRanges()
  {
    return invalidated_;
  }

  bool HasInvalidatedRanges()
  {
    return !invalidated_.isEmpty();
  }

  QString GetCacheDirectory() const;

public slots:
  void Invalidate(const TimeRange& r);

  void InvalidateAll();

  void SetLength(const rational& r);

  void Shift(const rational& from, const rational& to);

signals:
  void Invalidated(const OLIVE_NAMESPACE::TimeRange& r);

  void Validated(const OLIVE_NAMESPACE::TimeRange& r);

  void Shifted(const OLIVE_NAMESPACE::rational& from, const OLIVE_NAMESPACE::rational& to);

  void LengthChanged(const OLIVE_NAMESPACE::rational& r);

protected:
  void Validate(const TimeRange& r);

  virtual void LengthChangedEvent(const rational& old, const rational& newlen);

  virtual void InvalidateEvent(const TimeRange& range);

  virtual void ShiftEvent(const rational& from, const rational& to);

  Project* GetProject() const;

  struct JobIdentifier {
    TimeRange range;
    qint64 job_time;
  };

  QList<JobIdentifier> jobs_;

private:
  void RemoveRangeFromJobs(const TimeRange& remove);

  TimeRangeList invalidated_;

  rational length_;

};

OLIVE_NAMESPACE_EXIT

#endif // PLAYBACKCACHE_H
