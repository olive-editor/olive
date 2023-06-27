/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include <cstring>

#include "util/timecodefunctions.h"
#include "util/tests.h"

using namespace olive::core;

bool timecodefunctions_time_to_timecode_test()
{
  rational drop_frame_30(1001, 30000);

  std::string timecode = Timecode::time_to_timecode(rational(1), drop_frame_30, Timecode::kTimecodeDropFrame);
  if (strcmp(timecode.c_str(), "00:00:01;00") != 0) {
    return false;
  }

  return true;
}

bool timecodefunctions_time_to_timecode_test2()
{
  rational bizarre_timebase(156632219);

  std::string timecode = Timecode::time_to_timecode(rational(0), bizarre_timebase, Timecode::kTimecodeDropFrame);
  if (strcmp(timecode.c_str(), "INVALID TIMEBASE") != 0) {
    return false;
  }

  return true;
}

int main()
{
  Tester t;

  t.add("Timecode::time_to_timecode", timecodefunctions_time_to_timecode_test);
  t.add("Timecode::time_to_timecode2", timecodefunctions_time_to_timecode_test2);

  return t.exec();
}
