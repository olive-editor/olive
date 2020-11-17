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

#ifndef VIEWERQUEUE_H
#define VIEWERQUEUE_H

#include <QLinkedList>

#include "codec/frame.h"

OLIVE_NAMESPACE_ENTER

struct ViewerPlaybackFrame {
  rational timestamp;
  FramePtr frame;
};

class ViewerQueue : public std::list<ViewerPlaybackFrame> {
public:
  ViewerQueue() = default;

  void AppendTimewise(const ViewerPlaybackFrame& f, int playback_speed)
  {
    if (this->empty() || (this->back().timestamp < f.timestamp) == (playback_speed > 0)) {
      this->push_back(f);
    } else {
      for (iterator i=this->begin(); i!=this->end(); i++) {
        if ((i->timestamp > f.timestamp) == (playback_speed > 0)) {
          this->insert(i, f);
          break;
        }
      }
    }
  }

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERQUEUE_H
