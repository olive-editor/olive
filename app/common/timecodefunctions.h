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

#ifndef TIMECODEFUNCTIONS_H
#define TIMECODEFUNCTIONS_H

#include <QString>

#include "common/rational.h"

namespace olive {

enum TimecodeDisplay {
  kTimecodeFrames,
  kTimecodeSeconds,
  kFrames,
  kMilliseconds
};

TimecodeDisplay CurrentTimecodeDisplay();

/**
 * @brief Convert a timestamp (according to a rational timebase) to a user-friendly string representation
 */
QString timestamp_to_timecode(const int64_t &timestamp, const rational& timebase, const TimecodeDisplay& display, bool show_plus_if_positive = false);

int64_t time_to_timestamp(const rational& time, const rational& timebase);

rational timestamp_to_time(const int64_t& timestamp, const rational& timebase);

}

#endif // TIMECODEFUNCTIONS_H
