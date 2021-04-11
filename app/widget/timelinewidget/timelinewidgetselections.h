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

#ifndef TIMELINEWIDGETSELECTIONS_H
#define TIMELINEWIDGETSELECTIONS_H

#include <QHash>

#include "common/timerange.h"
#include "node/output/track/track.h"

namespace olive {

class TimelineWidgetSelections : public QHash<Track::Reference, TimeRangeList>
{
public:
  TimelineWidgetSelections() = default;

  void ShiftTime(const rational& diff);

  void ShiftTracks(Track::Type type, int diff);

  void TrimIn(const rational& diff);

  void TrimOut(const rational& diff);

};

}

#endif // TIMELINEWIDGETSELECTIONS_H
