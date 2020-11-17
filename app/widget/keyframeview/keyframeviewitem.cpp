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

#include "keyframeviewitem.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "common/qtutils.h"
#include "node/input.h"

namespace olive {

KeyframeViewItem::KeyframeViewItem(NodeKeyframePtr key, QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  key_(key),
  scale_(1.0),
  vert_center_(0),
  use_custom_brush_(false)
{
  setFlag(QGraphicsItem::ItemIsSelectable);

  connect(key.get(), &NodeKeyframe::TimeChanged, this, &KeyframeViewItem::UpdatePos);
  connect(key.get(), &NodeKeyframe::TypeChanged, this, &KeyframeViewItem::Redraw);

  int keyframe_size = QtUtils::QFontMetricsWidth(qApp->fontMetrics(), "Oi");
  int half_sz = keyframe_size/2;
  setRect(-half_sz, -half_sz, keyframe_size, keyframe_size);

  UpdatePos();

  // Set default brush
}

void KeyframeViewItem::SetOverrideY(qreal vertical_center)
{
  vert_center_ = vertical_center;
  UpdatePos();
}

void KeyframeViewItem::SetScale(double scale)
{
  scale_ = scale;
  UpdatePos();
}

void KeyframeViewItem::SetOverrideBrush(const QBrush &b)
{
  use_custom_brush_ = true;
  setBrush(b);
}

NodeKeyframePtr KeyframeViewItem::key() const
{
  return key_;
}

void KeyframeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->setRenderHint(QPainter::Antialiasing);

  painter->setPen(Qt::black);

  if (option->state & QStyle::State_Selected) {
    painter->setBrush(widget->palette().highlight());
  } else if (use_custom_brush_) {
    painter->setBrush(brush());
  } else {
    painter->setBrush(widget->palette().text());
  }

  switch (key_->type()) {
  case NodeKeyframe::kLinear:
  {
    QPointF points[] = {
      QPointF(rect().center().x(), rect().top()),
      QPointF(rect().right(), rect().center().y()),
      QPointF(rect().center().x(), rect().bottom()),
      QPointF(rect().left(), rect().center().y())
    };

    painter->drawPolygon(points, 4);
    break;
  }
  case NodeKeyframe::kBezier:
    painter->drawEllipse(rect());
    break;
  case NodeKeyframe::kHold:
    painter->drawRect(rect());
    break;
  }
}

void KeyframeViewItem::TimeTargetChangedEvent(Node *)
{
  UpdatePos();
}

void KeyframeViewItem::UpdatePos()
{
  rational adjusted = GetAdjustedTime(key_->parent()->parentNode(), GetTimeTarget(), key_->time(), NodeParam::kOutput);

  setPos(adjusted.toDouble() * scale_, vert_center_);
}

void KeyframeViewItem::Redraw()
{
  QGraphicsItem::update();
}

}
