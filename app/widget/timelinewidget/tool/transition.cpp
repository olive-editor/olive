#include "widget/timelinewidget/timelinewidget.h"

#include "node/block/transition/crossdissolve/crossdissolve.h"
#include "widget/nodeview/nodeviewundo.h"

TimelineWidget::TransitionTool::TransitionTool(TimelineWidget *parent) :
  AddTool(parent)
{
}

void TimelineWidget::TransitionTool::MousePress(TimelineViewMouseEvent *event)
{
  const TrackReference& track = event->GetCoordinates().GetTrack();
  TrackOutput* t = parent()->GetTrackFromReference(track);
  rational cursor_frame = event->GetCoordinates().GetFrame();

  if (!t || t->IsLocked()) {
    return;
  }

  Block* block_at_time = t->BlockAtTime(event->GetCoordinates().GetFrame());
  if (!block_at_time || block_at_time->type() != Block::kClip) {
    return;
  }

  // Determine which side of the clip the transition belongs to
  rational transition_start_point;
  olive::timeline::MovementMode trim_mode;
  rational halfway_point = block_at_time->in() + block_at_time->length() / 2;
  if (cursor_frame < halfway_point) {
    transition_start_point = block_at_time->in();
    trim_mode = olive::timeline::kTrimIn;
  } else {
    transition_start_point = block_at_time->out();
    trim_mode = olive::timeline::kTrimOut;
  }

  // Create ghost
  ghost_ = new TimelineViewGhostItem();
  ghost_->SetTrack(track);
  ghost_->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));
  ghost_->SetIn(transition_start_point);
  ghost_->SetOut(transition_start_point);
  ghost_->SetMode(trim_mode);
  ghost_->setData(TimelineViewGhostItem::kAttachedBlock, Node::PtrToValue(block_at_time));
  parent()->AddGhost(ghost_);

  snap_points_.append(transition_start_point);

  // Set the drag start point
  drag_start_point_ = cursor_frame;
}

void TimelineWidget::TransitionTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (!ghost_) {
    return;
  }

  MouseMoveInternal(event->GetCoordinates().GetFrame(), false);
}

void TimelineWidget::TransitionTool::MouseRelease(TimelineViewMouseEvent *event)
{
  MouseMove(event);

  const TrackReference& track = ghost_->Track();

  if (ghost_) {
    if (!ghost_->AdjustedLength().isNull()) {
      Block* block_to_transition = Node::ValueToPtr<Block>(ghost_->data(TimelineViewGhostItem::kAttachedBlock));
      CrossDissolveTransition* transition = new CrossDissolveTransition();
      NodeInput* transition_input_to_connect;

      if (ghost_->mode() == olive::timeline::kTrimIn) {
        transition->set_in_and_out_offset(ghost_->AdjustedLength(), 0);
        transition_input_to_connect = transition->in_block_input();
      } else {
        transition->set_in_and_out_offset(0, ghost_->AdjustedLength());
        transition_input_to_connect = transition->out_block_input();
      }

      QUndoCommand* command = new QUndoCommand();

      // Place transition in place
      new TrackPlaceBlockCommand(parent()->timeline_node_->track_list(track.type()),
                                                              track.index(),
                                                              transition,
                                                              ghost_->GetAdjustedIn(),
                                                              command);

      // Connect block to transition
      new NodeEdgeAddCommand(block_to_transition->output(),
                             transition_input_to_connect,
                             command);

      olive::undo_stack.push(command);
    }

    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
}
