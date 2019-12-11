#include "widget/timelinewidget/timelinewidget.h"

#include "core.h"

TimelineWidget::AddTool::AddTool(TimelineWidget *parent) :
  Tool(parent),
  ghost_(nullptr)
{
}

void TimelineWidget::AddTool::MousePress(TimelineViewMouseEvent *event)
{
  const TrackReference& track = event->GetCoordinates().GetTrack();
  TrackOutput* t = parent()->GetTrackFromReference(track);

  if (t && t->IsLocked()) {
    return;
  }

  ghost_ = new TimelineViewGhostItem();
  ghost_->SetIn(event->GetCoordinates().GetFrame());
  ghost_->SetOut(event->GetCoordinates().GetFrame());
  ghost_->SetTrack(track);
  ghost_->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));
  parent()->AddGhost(ghost_);

  snap_points_.append(event->GetCoordinates().GetFrame());
}

void TimelineWidget::AddTool::MouseMove(TimelineViewMouseEvent *event)
{
  const rational& cursor_frame = event->GetCoordinates().GetFrame();

  // Calculate movement
  rational movement = cursor_frame - ghost_->Out();

  // Snap movement
  SnapPoint(snap_points_, &movement);

  // Validation: Ensure in point never goes below 0
  if (movement < -ghost_->In()) {
    movement = -ghost_->In();
  }

  // Make adjustment
  if (!movement) {
    ghost_->SetInAdjustment(0);
    ghost_->SetOutAdjustment(0);
  } else if (movement > 0) {
    ghost_->SetInAdjustment(0);
    ghost_->SetOutAdjustment(movement);
  } else if (movement < 0) {
    ghost_->SetInAdjustment(movement);
    ghost_->SetOutAdjustment(0);
  }
}

void TimelineWidget::AddTool::MouseRelease(TimelineViewMouseEvent *event)
{
  MouseMove(event);

  const TrackReference& track = ghost_->Track();

  if (ghost_) {
    if (!ghost_->AdjustedLength().isNull()) {
      ClipBlock* clip = new ClipBlock();
      clip->set_length(ghost_->AdjustedLength());
      olive::undo_stack.push(new TrackPlaceBlockCommand(parent()->timeline_node_->track_list(track.type()),
                                                        track.index(),
                                                        clip,
                                                        ghost_->GetAdjustedIn()));
    }

    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
}
