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

#ifndef NODEVIEWCOMMON_H
#define NODEVIEWCOMMON_H

#include <QtGlobal>

#include "common/define.h"

namespace olive {

class NodeViewCommon {
public:
  enum FlowDirection {
    kTopToBottom,
    kBottomToTop,
    kLeftToRight,
    kRightToLeft
  };

  static Qt::Orientation GetFlowOrientation(FlowDirection dir) {
    if (dir == kTopToBottom || dir == kBottomToTop) {
      return Qt::Vertical;
    } else {
      return Qt::Horizontal;
    }
  }

  static bool DirectionsAreOpposing(FlowDirection a, FlowDirection b) {
    return ((a == NodeViewCommon::kLeftToRight && b == NodeViewCommon::kRightToLeft)
            || (a == NodeViewCommon::kRightToLeft && b == NodeViewCommon::kLeftToRight)
            || (a == NodeViewCommon::kTopToBottom && b == NodeViewCommon::kBottomToTop)
            || (a == NodeViewCommon::kBottomToTop && b == NodeViewCommon::kTopToBottom));
  }

};

}

#endif // NODEVIEWCOMMON_H
