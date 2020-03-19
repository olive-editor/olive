#include "timelineworkarea.h"

const rational TimelineWorkArea::kResetIn = 0;
const rational TimelineWorkArea::kResetOut = RATIONAL_MAX;

TimelineWorkArea::TimelineWorkArea(QObject *parent) :
  QObject(parent)
{
}

bool TimelineWorkArea::enabled() const
{
  return workarea_enabled_;
}

void TimelineWorkArea::set_enabled(bool e)
{
  workarea_enabled_ = e;
  emit EnabledChanged(workarea_enabled_);
}

const TimeRange &TimelineWorkArea::range() const
{
  return workarea_range_;
}

void TimelineWorkArea::set_range(const TimeRange &range)
{
  workarea_range_ = range;
  emit RangeChanged(workarea_range_);
}

const rational &TimelineWorkArea::in() const
{
  return workarea_range_.in();
}

const rational &TimelineWorkArea::out() const
{
  return workarea_range_.out();
}
