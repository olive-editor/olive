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

#include "timelineworkarea.h"

#include "common/xmlutils.h"

namespace olive {

const rational TimelineWorkArea::kResetIn = 0;
const rational TimelineWorkArea::kResetOut = RATIONAL_MAX;

TimelineWorkArea::TimelineWorkArea(QObject *parent) :
  QObject(parent),
  workarea_enabled_(false)
{
}

bool TimelineWorkArea::enabled() const
{
  return workarea_enabled_;
}

void TimelineWorkArea::set_enabled(bool e)
{
  workarea_enabled_ = e;
  emit EnabledChanged(workarea_enabled_);
}

const TimeRange &TimelineWorkArea::range() const
{
  return workarea_range_;
}

void TimelineWorkArea::set_range(const TimeRange &range)
{
  workarea_range_ = range;
  emit RangeChanged(workarea_range_);
}

const rational &TimelineWorkArea::in() const
{
  return workarea_range_.in();
}

const rational &TimelineWorkArea::out() const
{
  return workarea_range_.out();
}

const rational &TimelineWorkArea::length() const
{
  return workarea_range_.length();
}

}
