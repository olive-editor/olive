#include "timelineplayhead.h"

TimelinePlayhead::TimelinePlayhead()
{

}

QColor TimelinePlayhead::PlayheadColor()
{
  return playhead_color_;
}

QColor TimelinePlayhead::PlayheadHighlightColor()
{
  return playhead_highlight_color_;
}

void TimelinePlayhead::SetPlayheadColor(QColor c)
{
  playhead_color_ = c;
}

void TimelinePlayhead::SetPlayheadHighlightColor(QColor c)
{
  playhead_highlight_color_ = c;
}
