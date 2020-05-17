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

#ifndef VIEWERQUEUE_H
#define VIEWERQUEUE_H

#include <QLinkedList>

#include "codec/frame.h"

OLIVE_NAMESPACE_ENTER

struct ViewerPlaybackFrame {
  rational timestamp;
  FramePtr frame;
};

class ViewerQueue : public QLinkedList<ViewerPlaybackFrame> {
public:
  ViewerQueue() = default;

  void AppendTimewise(const ViewerPlaybackFrame& f, int playback_speed)
  {
    reverse_iterator i = this->rbegin();

    if (this->isEmpty() || (this->last().timestamp < f.timestamp) == (playback_speed > 0)) {
      this->append(f);
    } else {
      while (i != this->rend()) {
        reverse_iterator next = i + 1;

        if ((next->timestamp < f.timestamp) == (playback_speed > 0)) {
          this->insert(i.base(), f);

          return;
        } else {
          i = next;
        }
      }
    }
  }

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERQUEUE_H
