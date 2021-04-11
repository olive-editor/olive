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

#include "keyframeview.h"

namespace olive {

#define super KeyframeViewBase

KeyframeView::KeyframeView(QWidget *parent) :
  KeyframeViewBase(parent),
  max_scroll_(0)
{
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
}

void KeyframeView::SetElementY(const NodeInput &c, int y)
{
  qreal scene_y = mapToScene(mapFromGlobal(QPoint(0, y))).y();

  element_y_.insert(c, scene_y);

  for (auto it=item_map().cbegin(); it!=item_map().cend(); it++) {
    if (it.key()->key_track_ref().input() == c) {
      it.value()->SetOverrideY(scene_y);
    }
  }
}

void KeyframeView::wheelEvent(QWheelEvent *event)
{
  if (!HandleZoomFromScroll(event)) {
    KeyframeViewBase::wheelEvent(event);
  }
}

void KeyframeView::SceneRectUpdateEvent(QRectF &rect)
{
  rect.setY(0);
  rect.setHeight(max_scroll_);
}

KeyframeViewItem* KeyframeView::AddKeyframe(NodeKeyframe* key)
{
  KeyframeViewItem* item = super::AddKeyframe(key);
  item->SetOverrideY(element_y_.value(key->key_track_ref().input()));
  return item;
}

}
