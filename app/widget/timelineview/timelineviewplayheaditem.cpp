#include "timelineviewplayheaditem.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QtMath>

TimelineViewPlayheadItem::TimelineViewPlayheadItem(QGraphicsItem *parent) :
  TimelineViewRect(parent)
{

}

void TimelineViewPlayheadItem::SetPlayhead(const int64_t &playhead)
{
  playhead_ = playhead;

  UpdateRect();
}

void TimelineViewPlayheadItem::UpdateRect()
{
  double x = TimeToScreenCoord(rational(playhead_ * timebase_.numerator(), timebase_.denominator()));
  double width = TimeToScreenCoord(timebase_);

  setRect(0, 0, width, scene()->height());
  setPos(x, 0);
}

void TimelineViewPlayheadItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(option)
  Q_UNUSED(widget)

  // FIXME: Make adjustable through CSS
  painter->setPen(Qt::NoPen);
  painter->setBrush(QColor(255, 255, 255, 128));
  painter->drawRect(rect());

  painter->setPen(Qt::red);
  painter->setBrush(Qt::NoBrush);
  painter->drawLine(QLineF(rect().topLeft(), rect().bottomLeft()));
}
