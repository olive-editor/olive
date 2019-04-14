#include "timelineobject.h"

TimelineObject::TimelineObject()
{  
}

TimelineObject::~TimelineObject()
{
}

int TimelineObject::MediaWidth()
{
  return SequenceWidth();
}

int TimelineObject::MediaHeight()
{
  return SequenceHeight();
}

double TimelineObject::MediaFrameRate()
{
  return SequenceFrameRate();
}

NodeGraph *TimelineObject::pipeline()
{
  return &pipeline_;
}
