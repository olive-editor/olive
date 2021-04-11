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

#ifndef TIMELINECOMMON_H
#define TIMELINECOMMON_H

#include "common/define.h"
#include "common/rational.h"

namespace olive {

class Block;
class Track;

class Timeline {
public:
  enum MovementMode {
    kNone,
    kMove,
    kTrimIn,
    kTrimOut
  };

  static bool IsATrimMode(MovementMode mode) {return mode == kTrimIn || mode == kTrimOut;}

  struct EditToInfo {
    Track* track;
    rational nearest_time;
    Block* nearest_block;
  };

};

// FIXME: Hardcoded (but that might be okay here)
#define PLAYHEAD_COLOR Qt::red

}

#endif // TIMELINECOMMON_H
