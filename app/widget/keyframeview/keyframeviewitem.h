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

#ifndef KEYFRAMEVIEWITEM_H
#define KEYFRAMEVIEWITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"
#include "widget/timetarget/timetarget.h"

OLIVE_NAMESPACE_ENTER

class KeyframeViewItem : public QObject, public QGraphicsRectItem, public TimeTargetObject
{
  Q_OBJECT
public:
  KeyframeViewItem(NodeKeyframePtr key, QGraphicsItem *parent = nullptr);

  void SetOverrideY(qreal vertical_center);

  void SetScale(double scale);

  void SetOverrideBrush(const QBrush& b);

  NodeKeyframePtr key() const;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void TimeTargetChangedEvent(Node* ) override;

private:
  NodeKeyframePtr key_;

  double scale_;

  qreal vert_center_;

  bool use_custom_brush_;

private slots:
  void UpdatePos();

  void Redraw();

};

OLIVE_NAMESPACE_EXIT

#endif // KEYFRAMEVIEWITEM_H
