/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#ifndef TIMELINETOOLS_H
#define TIMELINETOOLS_H

namespace olive {
namespace timeline {

enum Tool {
  TIMELINE_TOOL_POINTER,
  TIMELINE_TOOL_EDIT,
  TIMELINE_TOOL_RAZOR,
  TIMELINE_TOOL_RIPPLE,
  TIMELINE_TOOL_ROLLING,
  TIMELINE_TOOL_SLIP,
  TIMELINE_TOOL_SLIDE,
  TIMELINE_TOOL_HAND,
  TIMELINE_TOOL_ZOOM,
  TIMELINE_TOOL_MENU,
  TIMELINE_TOOL_TRANSITION,
  TIMELINE_TOOL_COUNT
};

extern Tool current_tool;

}
}

#endif // TIMELINETOOLS_H
