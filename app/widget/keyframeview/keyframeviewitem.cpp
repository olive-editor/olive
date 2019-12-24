#include "keyframeviewitem.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

#include "common/qtversionabstraction.h"

KeyframeViewItem::KeyframeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  key_(nullptr),
  scale_(1.0)
{
  keyframe_size_ = QFontMetricsWidth(qApp->fontMetrics(), "Oi");
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemIsMovable);
}

void KeyframeViewItem::SetKeyframe(NodeKeyframe *key)
{
  key_ = key;
  UpdateRect();
}

void KeyframeViewItem::SetVerticalCenter(int middle)
{
  middle_ = middle;
  UpdateRect();
}

void KeyframeViewItem::SetScale(double scale)
{
  scale_ = scale;
  UpdateRect();
}

void KeyframeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  if (!key_) {
    return;
  }

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

void KeyframeViewItem::UpdateRect()
{
  if (!key_) {
    return;
  }

  double x_center = key_->time().toDouble() * scale_;

  setRect(x_center - keyframe_size_/2, middle_ - keyframe_size_/2, keyframe_size_, keyframe_size_);
}
