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

#include "beziercontrolpointitem.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "common/qtutils.h"

OLIVE_NAMESPACE_ENTER

BezierControlPointItem::BezierControlPointItem(NodeKeyframePtr key, NodeKeyframe::BezierType mode, QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  key_(key),
  mode_(mode),
  x_scale_(1.0),
  y_scale_(1.0)
{
  setFlag(QGraphicsItem::ItemIsMovable);

  connect(key.get(), &NodeKeyframe::TimeChanged, this, &BezierControlPointItem::UpdatePos);

  if (mode_ == NodeKeyframe::kInHandle) {
    connect(key.get(), &NodeKeyframe::BezierControlInChanged, this, &BezierControlPointItem::UpdatePos);
  } else {
    connect(key.get(), &NodeKeyframe::BezierControlOutChanged, this, &BezierControlPointItem::UpdatePos);
  }


  int control_point_size = QFontMetricsWidth(qApp->fontMetrics(), "o");
  int half_sz = control_point_size / 2;
  setRect(-half_sz, -half_sz, control_point_size, control_point_size);
}

void BezierControlPointItem::SetXScale(double scale)
{
  x_scale_ = scale;
  UpdatePos();
}

void BezierControlPointItem::SetYScale(double scale)
{
  y_scale_ = scale;
  UpdatePos();
}

NodeKeyframePtr BezierControlPointItem::key() const
{
  return key_;
}

const NodeKeyframe::BezierType &BezierControlPointItem::mode() const
{
  return mode_;
}

QPointF BezierControlPointItem::GetCorrespondingKeyframeHandle() const
{
  return key_->bezier_control(mode_);
}

void BezierControlPointItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  if (option->state & QStyle::State_Selected) {
    painter->setPen(widget->palette().highlight().color());
  } else {
    painter->setPen(widget->palette().text().color());
  }

  painter->drawEllipse(rect());
}

void BezierControlPointItem::UpdatePos()
{
  QPointF handle_offset = GetCorrespondingKeyframeHandle();

  // Scale handle offset
  handle_offset.setX(handle_offset.x() * x_scale_);

  // Flip the Y coordinate because bezier curves are drawn bottom to top
  handle_offset.setY(-handle_offset.y() * y_scale_);

  setPos(handle_offset - rect().center());
}

OLIVE_NAMESPACE_EXIT
