#include "widget/timelinewidget/timelinewidget.h"

#include "node/block/transition/transition.h"
#include "node/factory.h"
#include "widget/nodeview/nodeviewundo.h"

TimelineWidget::TransitionTool::TransitionTool(TimelineWidget *parent) :
  AddTool(parent)
{
}

void TimelineWidget::TransitionTool::MousePress(TimelineViewMouseEvent *event)
{
  const TrackReference& track = event->GetTrack();
  TrackOutput* t = parent()->GetTrackFromReference(track);
  rational cursor_frame = event->GetFrame();

  if (!t || t->IsLocked()) {
    return;
  }

  Block* block_at_time = t->BlockAtTime(event->GetFrame());
  if (!block_at_time || block_at_time->type() != Block::kClip) {
    return;
  }

  // Determine which side of the clip the transition belongs to
  rational transition_start_point;
  Timeline::MovementMode trim_mode;
  rational halfway_point = block_at_time->in() + block_at_time->length() / 2;
  rational tenth_point = block_at_time->in() + block_at_time->length() / 10;
  Block* other_block = nullptr;
  if (cursor_frame < halfway_point) {
    transition_start_point = block_at_time->in();
    trim_mode = Timeline::kTrimIn;

    if (cursor_frame < tenth_point
        && block_at_time->previous()
        && block_at_time->previous()->type() == Block::kClip) {
      other_block = block_at_time->previous();
    }
  } else {
    transition_start_point = block_at_time->out();
    trim_mode = Timeline::kTrimOut;
    dual_transition_ = (cursor_frame > block_at_time->length() - tenth_point);

    if (cursor_frame > block_at_time->length() - tenth_point
        && block_at_time->next()
        && block_at_time->next()->type() == Block::kClip) {
      other_block = block_at_time->next();
    }
  }

  // Create ghost
  ghost_ = new TimelineViewGhostItem();
  ghost_->SetTrack(track);
  ghost_->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));
  ghost_->SetIn(transition_start_point);
  ghost_->SetOut(transition_start_point);
  ghost_->SetMode(trim_mode);
  ghost_->setData(TimelineViewGhostItem::kAttachedBlock, Node::PtrToValue(block_at_time));

  dual_transition_ = (other_block);
  if (other_block)
    ghost_->setData(TimelineViewGhostItem::kReferenceBlock, Node::PtrToValue(other_block));

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

  MouseMoveInternal(event->GetFrame(), dual_transition_);
}

void TimelineWidget::TransitionTool::MouseRelease(TimelineViewMouseEvent *event)
{
  MouseMove(event);

  const TrackReference& track = ghost_->Track();

  if (ghost_) {
    if (!ghost_->AdjustedLength().isNull()) {
      TransitionBlock* transition = static_cast<TransitionBlock*>(NodeFactory::CreateFromID("org.olivevideoeditor.Olive.crossdissolve"));

      QUndoCommand* command = new QUndoCommand();

      // Place transition in place
      new NodeAddCommand(static_cast<NodeGraph*>(parent()->GetConnectedNode()->parent()),
                         transition,
                         command);

      new TrackPlaceBlockCommand(parent()->GetConnectedNode()->track_list(track.type()),
                                 track.index(),
                                 transition,
                                 ghost_->GetAdjustedIn(),
                                 command);

      if (dual_transition_) {
        transition->set_length_and_media_out(ghost_->AdjustedLength());
        transition->set_media_in(-ghost_->AdjustedLength()/2);

        // Block mouse is hovering over
        Block* active_block = Node::ValueToPtr<Block>(ghost_->data(TimelineViewGhostItem::kAttachedBlock));

        // Block mouse is next to
        Block* friend_block = Node::ValueToPtr<Block>(ghost_->data(TimelineViewGhostItem::kReferenceBlock));

        // Use ghost mode to determine which block is which
        Block* out_block = (ghost_->mode() == Timeline::kTrimIn) ? friend_block : active_block;
        Block* in_block = (ghost_->mode() == Timeline::kTrimIn) ? active_block : friend_block;

        // Connect block to transition
        new NodeEdgeAddCommand(out_block->output(),
                               transition->out_block_input(),
                               command);

        new NodeEdgeAddCommand(in_block->output(),
                               transition->in_block_input(),
                               command);
      } else {
        Block* block_to_transition = Node::ValueToPtr<Block>(ghost_->data(TimelineViewGhostItem::kAttachedBlock));
        NodeInput* transition_input_to_connect;

        if (ghost_->mode() == Timeline::kTrimIn) {
          transition->set_length_and_media_out(ghost_->AdjustedLength());
          transition_input_to_connect = transition->in_block_input();
        } else {
          transition->set_length_and_media_out(ghost_->AdjustedLength());
          transition_input_to_connect = transition->out_block_input();
        }

        // Connect block to transition
        new NodeEdgeAddCommand(block_to_transition->output(),
                               transition_input_to_connect,
                               command);
      }

      Core::instance()->undo_stack()->push(command);
    }

    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
}
