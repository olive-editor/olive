#include "timelineviewclipitem.h"

#include <QBrush>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

TimelineViewClipItem::TimelineViewClipItem(QGraphicsItem* parent) :
  QGraphicsRectItem(parent),
  clip_(nullptr)
{
  setBrush(Qt::white);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void TimelineViewClipItem::SetClip(ClipBlock *clip)
{
  clip_ = clip;
}

void TimelineViewClipItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  /*QLinearGradient grad;
  grad.setStart(0, 0);
  grad.setFinalStop(0, rect().height());
  grad.setColorAt(0.0, QColor(192, 192, 255));
  grad.setColorAt(1.0, QColor(128, 128, 192));
  painter->fillRect(rect(), grad);*/
  painter->fillRect(rect(), QColor(128, 128, 192));

  if (option->state & QStyle::State_Selected) {
    painter->fillRect(rect(), QColor(0, 0, 0, 64));
  }

  painter->setPen(Qt::white);
  painter->drawLine(rect().topLeft(), QPointF(rect().right(), rect().top()));
  painter->drawLine(rect().topLeft(), QPointF(rect().left(), rect().bottom() - 1));

  painter->setPen(QColor(64, 64, 64));
  painter->drawLine(QPointF(rect().left(), rect().bottom() - 1), QPointF(rect().right(), rect().bottom() - 1));
  painter->drawLine(QPointF(rect().right(), rect().bottom() - 1), QPointF(rect().right(), rect().top()));
}
