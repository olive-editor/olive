#include "widget/timelinewidget/timelinewidget.h"

TimelineWidget::EditTool::EditTool(TimelineWidget* parent) :
  Tool(parent)
{
}

void TimelineWidget::EditTool::MousePress(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineWidget::EditTool::MouseMove(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineWidget::EditTool::MouseRelease(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}
