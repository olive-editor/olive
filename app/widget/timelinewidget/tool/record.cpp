#include "record.h"

#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

RecordTool::RecordTool(TimelineWidget *parent) :
  BeamTool(parent),
  ghost_(nullptr)
{

}

void RecordTool::MousePress(TimelineViewMouseEvent *event)
{
  const Track::Reference& track = event->GetTrack();

  // Check if track is locked
  Track* t = parent()->GetTrackFromReference(track);
  if (t && t->IsLocked()) {
    return;
  }

  if (t->type() != Track::kAudio) {
    // We only support audio tracks here
    return;
  }

  drag_start_point_ = ValidatedCoordinate(event->GetCoordinates(true)).GetFrame();

  ghost_ = new TimelineViewGhostItem();
  ghost_->SetIn(drag_start_point_);
  ghost_->SetOut(drag_start_point_);
  ghost_->SetTrack(track);
  parent()->AddGhost(ghost_);

  snap_points_.push_back(drag_start_point_);
}

void RecordTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (!ghost_) {
    return;
  }

  // Calculate movement
  rational movement = event->GetFrame() - drag_start_point_;

  // Validation: Ensure in point never goes below 0
  if (movement < -ghost_->GetIn()) {
    movement = -ghost_->GetIn();
  }

  // Snap movement
  bool snapped;

  if (Core::instance()->snapping()) {
    snapped = parent()->SnapPoint(snap_points_, &movement);
  } else {
    snapped = false;
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

  Q_UNUSED(snapped)
}

void RecordTool::MouseRelease(TimelineViewMouseEvent *event)
{
  if (ghost_) {
    emit parent()->RequestCaptureStart(TimeRange(ghost_->GetAdjustedIn(), ghost_->GetAdjustedOut()), ghost_->GetTrack());
    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
}

}
