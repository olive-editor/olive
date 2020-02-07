#include "keyframeviewitem.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "common/qtutils.h"

KeyframeViewItem::KeyframeViewItem(NodeKeyframePtr key, QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  key_(key),
  scale_(1.0),
  vert_center_(0)
{
  setFlag(QGraphicsItem::ItemIsSelectable);

  connect(key.get(), &NodeKeyframe::TimeChanged, this, &KeyframeViewItem::UpdatePos);
  connect(key.get(), &NodeKeyframe::TypeChanged, this, &KeyframeViewItem::Redraw);

  int keyframe_size = QFontMetricsWidth(qApp->fontMetrics(), "Oi");
  int half_sz = keyframe_size/2;
  setRect(-half_sz, -half_sz, keyframe_size, keyframe_size);

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

void KeyframeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  painter->setRenderHint(QPainter::Antialiasing);

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

  setPos(x_center, vert_center_);
}

void KeyframeViewItem::Redraw()
{
  QGraphicsItem::update();
}
