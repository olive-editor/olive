#include "keyframeviewitem.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "common/qtversionabstraction.h"

KeyframeViewItem::KeyframeViewItem(NodeKeyframePtr key, QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  key_(key),
  scale_(1.0),
  vert_center_(0)
{
  keyframe_size_ = QFontMetricsWidth(qApp->fontMetrics(), "Oi");
  setFlag(QGraphicsItem::ItemIsSelectable);

  connect(key.get(), &NodeKeyframe::TimeChanged, this, &KeyframeViewItem::UpdatePos);
  connect(key.get(), &NodeKeyframe::TypeChanged, this, &KeyframeViewItem::Redraw);

  setRect(0, 0, keyframe_size_, keyframe_size_);

  UpdatePos();
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

NodeKeyframePtr KeyframeViewItem::key() const
{
  return key_;
}

QPointF KeyframeViewItem::center_pos() const
{
  return pos() + rect().center();
}

void KeyframeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->setPen(Qt::black);

  if (option->state & QStyle::State_Selected) {
    painter->setBrush(widget->palette().highlight());
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

void KeyframeViewItem::UpdatePos()
{
  double x_center = key_->time().toDouble() * scale_;

  setPos(x_center - keyframe_size_/2, vert_center_ - keyframe_size_/2);
}

void KeyframeViewItem::Redraw()
{
  QGraphicsItem::update();
}
