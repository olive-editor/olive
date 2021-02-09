/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "node/factory.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/text.h"
#include "widget/timelinewidget/timelinewidget.h"

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
  case olive::Tool::kAddableBars:
  case olive::Tool::kAddableSolid:
  case olive::Tool::kAddableTitle:
    add_type = Track::kVideo;
    break;
  case olive::Tool::kAddableTone:
    add_type = Track::kAudio;
    break;
  case olive::Tool::kAddableEmpty:
    // Leave as "none", which means this block can be placed on any track
    break;
  case olive::Tool::kAddableCount:
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

    snap_points_.append(drag_start_point_);
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
  const Track::Reference& track = ghost_->GetTrack();

  if (ghost_) {
    if (!ghost_->GetAdjustedLength().isNull()) {
      MultiUndoCommand* command = new MultiUndoCommand();

      ClipBlock* clip = new ClipBlock();
      clip->set_length_and_media_out(ghost_->GetAdjustedLength());
      clip->SetLabel(olive::Tool::GetAddableObjectName(Core::instance()->GetSelectedAddableObject()));

      NodeGraph* graph = static_cast<NodeGraph*>(parent()->GetConnectedNode()->parent());

      command->add_child(new NodeAddCommand(graph,
                                            clip));

      command->add_child(new TrackPlaceBlockCommand(parent()->GetConnectedNode()->track_list(track.type()),
                                                    track.index(),
                                                    clip,
                                                    ghost_->GetAdjustedIn()));

      switch (Core::instance()->GetSelectedAddableObject()) {
      case olive::Tool::kAddableEmpty:
        // Empty, nothing to be done
        break;
      case olive::Tool::kAddableSolid:
      {
        Node* solid = new SolidGenerator();

        command->add_child(new NodeAddCommand(graph,
                                              solid));

        command->add_child(new NodeEdgeAddCommand(solid, NodeInput(clip, ClipBlock::kBufferIn)));
        break;
      }
      case olive::Tool::kAddableTitle:
      {
        Node* text = new TextGenerator();

        command->add_child(new NodeAddCommand(graph,
                                              text));

        command->add_child(new NodeEdgeAddCommand(text, NodeInput(clip, ClipBlock::kBufferIn)));
        break;
      }
      case olive::Tool::kAddableBars:
      case olive::Tool::kAddableTone:
        // Not implemented yet
        qWarning() << "Unimplemented add object:" << Core::instance()->GetSelectedAddableObject();
        break;
      case olive::Tool::kAddableCount:
        // Invalid value, do nothing
        break;
      }

      Core::instance()->undo_stack()->push(command);
    }

    parent()->ClearGhosts();
    snap_points_.clear();
    ghost_ = nullptr;
  }
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
