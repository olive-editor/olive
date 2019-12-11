#include "widget/timelinewidget/timelinewidget.h"

TimelineWidget::TransitionTool::TransitionTool(TimelineWidget *parent) :
  Tool(parent)
{
}

void TimelineWidget::TransitionTool::MousePress(TimelineViewMouseEvent *event)
{
  TimelineViewBlockItem* item = GetItemAtScenePos(event->GetCoordinates());

  bool selectable_item = (item != nullptr
      && item->flags() & QGraphicsItem::ItemIsSelectable
      && !parent()->GetTrackFromReference(item->Track())->IsLocked());

  if (!selectable_item) {
    return;
  }
}

void TimelineWidget::TransitionTool::MouseMove(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}

void TimelineWidget::TransitionTool::MouseRelease(TimelineViewMouseEvent *event)
{
  Q_UNUSED(event)
}
