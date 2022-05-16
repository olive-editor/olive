/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "add.h"
#include "core.h"
#include "node/block/subtitle/subtitle.h"
#include "node/factory.h"
#include "node/generator/shape/shapenode.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv3.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/timelinewidget/timelinewidget.h"
#include "widget/timelinewidget/undo/timelineundopointer.h"

namespace olive {

AddTool::AddTool(TimelineWidget *parent) :
  BeamTool(parent),
  ghost_(nullptr)
{
}

void AddTool::MousePress(TimelineViewMouseEvent *event)
{
  const Track::Reference& track = event->GetTrack();

  // Check if track is locked
  Track* t = parent()->GetTrackFromReference(track);
  if (t && t->IsLocked()) {
    return;
  }

  Track::Type add_type = Track::kNone;

  switch (Core::instance()->GetSelectedAddableObject()) {
  case Tool::kAddableBars:
  case Tool::kAddableSolid:
  case Tool::kAddableTitle:
  case Tool::kAddableShape:
    add_type = Track::kVideo;
    break;
  case Tool::kAddableTone:
    add_type = Track::kAudio;
    break;
  case Tool::kAddableSubtitle:
    add_type = Track::kSubtitle;
    break;
  case Tool::kAddableEmpty:
    // Leave as "none", which means this block can be placed on any track
    break;
  case Tool::kAddableCount:
    // Return so we do nothing
    return;
  }

  if (add_type == Track::kNone
      || add_type == track.type()) {
    drag_start_point_ = ValidatedCoordinate(event->GetCoordinates(true)).GetFrame();

    ghost_ = new TimelineViewGhostItem();
    ghost_->SetIn(drag_start_point_);
    ghost_->SetOut(drag_start_point_);
    ghost_->SetTrack(track);
    parent()->AddGhost(ghost_);

    snap_points_.push_back(drag_start_point_);
  }
}

void AddTool::MouseMove(TimelineViewMouseEvent *event)
{
  if (!ghost_) {
    return;
  }

  MouseMoveInternal(event->GetFrame(), event->GetModifiers() & Qt::AltModifier);
}

void AddTool::MouseRelease(TimelineViewMouseEvent *event)
{
  if (ghost_) {
    if (!ghost_->GetAdjustedLength().isNull()) {
      MultiUndoCommand* command = new MultiUndoCommand();

      if (MultiUndoCommand *subtitle_section_command = parent()->TakeSubtitleSectionCommand()) {
        command->add_child(subtitle_section_command);
      }

      Sequence *s = parent()->sequence();

      QRectF r;
      if (Core::instance()->GetSelectedAddableObject() == Tool::kAddableTitle) {
        VideoParams svp = s->GetVideoParams();
        r = QRectF(0, 0, svp.width(), svp.height());
        r.adjust(svp.width()/10, svp.height()/10, -svp.width()/10, -svp.height()/10);
      }

      CreateAddableClip(command, s, ghost_->GetTrack(), ghost_->GetAdjustedIn(), ghost_->GetAdjustedLength(), r);

      Core::instance()->undo_stack()->push(command);
    }

    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
}

Node *AddTool::CreateAddableClip(MultiUndoCommand *command, Sequence *sequence, const Track::Reference &track, const rational &in, const rational &length, const QRectF &rect)
{
  ClipBlock* clip;
  if (Core::instance()->GetSelectedAddableObject() == Tool::kAddableSubtitle) {
    clip = new SubtitleBlock();
  } else {
    clip = new ClipBlock();
  }
  clip->set_length_and_media_out(length);
  clip->SetLabel(olive::Tool::GetAddableObjectName(Core::instance()->GetSelectedAddableObject()));

  NodeGraph* graph = sequence->parent();

  command->add_child(new NodeAddCommand(graph, clip));
  command->add_child(new NodeSetPositionCommand(clip, clip, QPointF(0, 0)));
  command->add_child(new TrackPlaceBlockCommand(sequence->track_list(track.type()),
                                                track.index(),
                                                clip,
                                                in));

  Node *node_to_add = nullptr;

  switch (Core::instance()->GetSelectedAddableObject()) {
  case Tool::kAddableEmpty:
    // Empty, nothing to be done
    break;
  case Tool::kAddableSolid:
    node_to_add = new SolidGenerator();
    break;
  case Tool::kAddableShape:
    node_to_add = new ShapeNode();
    break;
  case Tool::kAddableTitle:
    node_to_add = new TextGeneratorV3();
    break;
  case Tool::kAddableBars:
  case Tool::kAddableTone:
    // Not implemented yet
    qWarning() << "Unimplemented add object:" << Core::instance()->GetSelectedAddableObject();
    break;
  case Tool::kAddableSubtitle:
    // The block itself is the node we want
    break;
  case Tool::kAddableCount:
    // Invalid value, do nothing
    break;
  }

  if (node_to_add) {
    QPointF extra_node_offset(kDefaultDistanceFromOutput, 0);
    command->add_child(new NodeAddCommand(graph, node_to_add));
    command->add_child(new NodeEdgeAddCommand(node_to_add, NodeInput(clip, ClipBlock::kBufferIn)));
    command->add_child(new NodeSetPositionCommand(node_to_add, clip, extra_node_offset));

    if (!rect.isNull()) {
      if (ShapeNodeBase *shape = dynamic_cast<ShapeNodeBase*>(node_to_add)) {
        shape->SetRect(rect, sequence->GetVideoParams(), command);
      }
    }
  }

  return node_to_add;
}

void AddTool::MouseMoveInternal(const rational &cursor_frame, bool outwards)
{
  // Calculate movement
  rational movement = cursor_frame - drag_start_point_;

  // Validation: Ensure in point never goes below 0
  if (movement < -ghost_->GetIn() || (outwards && -movement < -ghost_->GetIn())) {
    movement = -ghost_->GetIn();
  }

  // Snap movement
  bool snapped;

  if (Core::instance()->snapping()) {
    snapped = parent()->SnapPoint(snap_points_, &movement);
  } else {
    snapped = false;
  }

  // If alt is held, our movement goes both ways (outwards)
  if (!snapped && outwards) {
    // Snap backwards too
    movement = -movement;
    parent()->SnapPoint(snap_points_, &movement);
    // We don't need to un-neg here because outwards means all future processing will be done both pos and neg
  }

  // Make adjustment
  if (!movement) {
    ghost_->SetInAdjustment(0);
    ghost_->SetOutAdjustment(0);
  } else if (movement > 0) {
    ghost_->SetInAdjustment(outwards ? -movement : 0);
    ghost_->SetOutAdjustment(movement);
  } else if (movement < 0) {
    ghost_->SetInAdjustment(movement);
    ghost_->SetOutAdjustment(outwards ? -movement : 0);
  }
}

}
