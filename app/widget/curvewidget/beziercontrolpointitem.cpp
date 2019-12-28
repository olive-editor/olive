#include "beziercontrolpointitem.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "common/qtversionabstraction.h"

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
  handle_offset.setY(handle_offset.y() * y_scale_);

  setPos(handle_offset - rect().center());
}
