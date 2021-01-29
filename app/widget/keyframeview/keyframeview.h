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

#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include "keyframeviewbase.h"

namespace olive {

class KeyframeView : public KeyframeViewBase
{
  Q_OBJECT
public:
  KeyframeView(QWidget* parent = nullptr);

  void SetMaxScroll(int i)
  {
    max_scroll_ = i;
  }

  void SetElementY(const Node::InputConnection& c, int y);

protected:
  virtual void wheelEvent(QWheelEvent* event) override;

  virtual void SceneRectUpdateEvent(QRectF& rect) override;

public slots:
  virtual KeyframeViewItem* AddKeyframe(NodeKeyframe* key) override;

private:
  QHash<Node::InputConnection, qreal> element_y_;

  int max_scroll_;

};

}

#endif // KEYFRAMEVIEW_H
