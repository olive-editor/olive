#include "seekablewidget.h"

#include <QMouseEvent>
#include <QtMath>

SeekableWidget::SeekableWidget(QWidget* parent) :
  TimelineScaledWidget(parent),
  time_(0),
  timeline_points_(nullptr),
  scroll_(0)
{

}

void SeekableWidget::ConnectTimelinePoints(TimelinePoints *points)
{
  if (timeline_points_) {
    disconnect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    disconnect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
  }

  timeline_points_ = points;

  if (timeline_points_) {
    connect(timeline_points_->workarea(), &TimelineWorkArea::RangeChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
    connect(timeline_points_->workarea(), &TimelineWorkArea::EnabledChanged, this, static_cast<void (SeekableWidget::*)()>(&SeekableWidget::update));
  }

  update();
}

const int64_t &SeekableWidget::GetTime() const
{
  return time_;
}

const int &SeekableWidget::GetScroll() const
{
  return scroll_;
}

void SeekableWidget::mousePressEvent(QMouseEvent *event)
{
  SeekToScreenPoint(event->pos().x());
}

void SeekableWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    SeekToScreenPoint(event->pos().x());
  }
}

void SeekableWidget::ScaleChangedEvent(const double &)
{
  update();
}

TimelinePoints *SeekableWidget::timeline_points() const
{
  return timeline_points_;
}

void SeekableWidget::SetTime(const int64_t &r)
{
  time_ = r;

  update();
}

void SeekableWidget::SetScroll(int s)
{
  scroll_ = s;

  update();
}

double SeekableWidget::ScreenToUnitFloat(int screen)
{
  return (screen + scroll_) / GetScale() / timebase_dbl();
}

int64_t SeekableWidget::ScreenToUnit(int screen)
{
  return qFloor(ScreenToUnitFloat(screen));
}

int64_t SeekableWidget::ScreenToUnitRounded(int screen)
{
  return qRound64(ScreenToUnitFloat(screen));
}

int SeekableWidget::UnitToScreen(int64_t unit)
{
  return qFloor(static_cast<double>(unit) * GetScale() * timebase_dbl()) - scroll_;
}

int SeekableWidget::TimeToScreen(const rational &time)
{
  return qFloor(time.toDouble() * GetScale()) - scroll_;
}

void SeekableWidget::SeekToScreenPoint(int screen)
{
  int64_t timestamp = qMax(static_cast<int64_t>(0), ScreenToUnitRounded(screen));

  SetTime(timestamp);

  emit TimeChanged(timestamp);
}
