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

#include <QObject>

#include "common/timerange.h"

OLIVE_NAMESPACE_ENTER

class PlaybackCache : public QObject
{
    Q_OBJECT
public:
  PlaybackCache();

  void Invalidate(const TimeRange& r);

  const rational& GetLength() const
  {
    return length_;
  }

  void SetLength(const rational& r);

  bool IsFullyValidated() const;

  const TimeRangeList& GetInvalidatedRanges() const
  {
    return invalidated_;
  }

signals:
  void Invalidated(const TimeRange& r);

  void Validated(const TimeRange& r);

  void LengthChanged(const rational& r);

protected:
  void Validate(const TimeRange& r);

  void InvalidateAll();

  virtual void LengthChangedEvent(const rational& old, const rational& newlen);

  virtual void InvalidateEvent(const TimeRange& range);

private:
  TimeRangeList invalidated_;

  rational length_;

};

OLIVE_NAMESPACE_EXIT

#endif // PLAYBACKCACHE_H
