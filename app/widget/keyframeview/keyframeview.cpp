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

OLIVE_NAMESPACE_ENTER

KeyframeView::KeyframeView(QWidget *parent) :
  KeyframeViewBase(parent),
  max_scroll_(0)
{
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
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

void KeyframeView::AddKeyframe(NodeKeyframePtr key, int y)
{
  QPoint global_pt(0, y);
  QPoint local_pt = mapFromGlobal(global_pt);
  QPointF scene_pt = mapToScene(local_pt);

  KeyframeViewItem* item = AddKeyframeInternal(key);
  item->SetOverrideY(scene_pt.y());
}

OLIVE_NAMESPACE_EXIT
