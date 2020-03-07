#include "timelinepoints.h"

TimelineMarkerList *TimelinePoints::markers()
{
  return &markers_;
}

const TimelineMarkerList *TimelinePoints::markers() const
{
  return &markers_;
}

const TimelineWorkArea *TimelinePoints::workarea() const
{
  return &workarea_;
}

TimelineWorkArea *TimelinePoints::workarea()
{
  return &workarea_;
}
