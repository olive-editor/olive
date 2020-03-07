#ifndef TIMELINEPOINTS_H
#define TIMELINEPOINTS_H

#include "timelinemarker.h"
#include "timelineworkarea.h"

class TimelinePoints
{
public:
  TimelinePoints() = default;

  TimelineMarkerList* markers();
  const TimelineMarkerList* markers() const;

  TimelineWorkArea* workarea();
  const TimelineWorkArea* workarea() const;

private:
  TimelineMarkerList markers_;

  TimelineWorkArea workarea_;

};

#endif // TIMELINEPOINTS_H
