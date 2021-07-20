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

#ifndef PLAYBACKCACHE_H
#define PLAYBACKCACHE_H

#include <QMutex>
#include <QObject>

#include "common/jobtime.h"
#include "common/timerange.h"

namespace olive {

class Project;
class ViewerOutput;

class PlaybackCache : public QObject
{
  Q_OBJECT
public:
  PlaybackCache(QObject* parent = nullptr) :
    QObject(parent)
  {
  }

  TimeRangeList GetInvalidatedRanges(TimeRange intersecting);
  TimeRangeList GetInvalidatedRanges(const rational &length)
  {
    return GetInvalidatedRanges(TimeRange(0, length));
  }

  bool HasInvalidatedRanges(const TimeRange &intersecting);
  bool HasInvalidatedRanges(const rational &length)
  {
    return HasInvalidatedRanges(TimeRange(0, length));
  }

  QString GetCacheDirectory() const;

  ViewerOutput *viewer_parent() const;

  void Invalidate(const TimeRange& r, bool signal = true);

public slots:
  void InvalidateAll();

  void Shift(rational from, rational to);

signals:
  void Invalidated(const olive::TimeRange& r);

  void Validated(const olive::TimeRange& r);

  void Shifted(const olive::rational& from, const olive::rational& to);

protected:
  void Validate(const TimeRange& r, bool signal = true);

  virtual void InvalidateEvent(const TimeRange& range);

  virtual void ShiftEvent(const rational& from, const rational& to);

  Project* GetProject() const;

private:
  TimeRangeList validated_;

};

}

#endif // PLAYBACKCACHE_H
