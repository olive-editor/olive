/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "widget/timelinewidget/timelinewidget.h"

#include "add.h"
#include "core.h"
#include "node/factory.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/text.h"
#include "widget/nodeview/nodeviewundo.h"

OLIVE_NAMESPACE_ENTER

AddTool::AddTool(TimelineWidget *parent) :
  BeamTool(parent),
  ghost_(nullptr)
{
}

void AddTool::MousePress(TimelineViewMouseEvent *event)
{
  const TrackReference& track = event->GetTrack();

  // Check if track is locked
  TrackOutput* t = parent()->GetTrackFromReference(track);
  if (t && t->IsLocked()) {
    return;
  }

  Timeline::TrackType add_type = Timeline::kTrackTypeNone;

  switch (Core::instance()->GetSelectedAddableObject()) {
  case OLIVE_NAMESPACE::Tool::kAddableBars:
  case OLIVE_NAMESPACE::Tool::kAddableSolid:
  case OLIVE_NAMESPACE::Tool::kAddableTitle:
    add_type = Timeline::kTrackTypeVideo;
    break;
  case OLIVE_NAMESPACE::Tool::kAddableTone:
    add_type = Timeline::kTrackTypeAudio;
    break;
  case OLIVE_NAMESPACE::Tool::kAddableEmpty:
    // Leave as "none", which means this block can be placed on any track
    break;
  case OLIVE_NAMESPACE::Tool::kAddableCount:
    // Return so we do nothing
    return;
  }

  if (add_type == Timeline::kTrackTypeNone
      || add_type == track.type()) {
    drag_start_point_ = ValidatedCoordinate(event->GetCoordinates(true)).GetFrame();

    ghost_ = new TimelineViewGhostItem();
    ghost_->SetIn(drag_start_point_);
    ghost_->SetOut(drag_start_point_);
    ghost_->SetTrack(track);
    ghost_->SetYCoords(parent()->GetTrackY(track), parent()->GetTrackHeight(track));
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
  const TrackReference& track = ghost_->Track();

  if (ghost_) {
    if (!ghost_->GetAdjustedLength().isNull()) {
      QUndoCommand* command = new QUndoCommand();

      ClipBlock* clip = new ClipBlock();
      clip->set_length_and_media_out(ghost_->GetAdjustedLength());
      clip->SetLabel(OLIVE_NAMESPACE::Tool::GetAddableObjectName(Core::instance()->GetSelectedAddableObject()));

      NodeGraph* graph = static_cast<NodeGraph*>(parent()->GetConnectedNode()->parent());

      new NodeAddCommand(graph,
                         clip,
                         command);

      new TrackPlaceBlockCommand(parent()->GetConnectedNode()->track_list(track.type()),
                                 track.index(),
                                 clip,
                                 ghost_->GetAdjustedIn(),
                                 command);

      switch (Core::instance()->GetSelectedAddableObject()) {
      case OLIVE_NAMESPACE::Tool::kAddableEmpty:
        // Empty, nothing to be done
        break;
      case OLIVE_NAMESPACE::Tool::kAddableSolid:
      {
        Node* solid = new SolidGenerator();

        new NodeAddCommand(graph,
                           solid,
                           command);

        new NodeEdgeAddCommand(solid->output(), clip->texture_input(), command);
        break;
      }
      case OLIVE_NAMESPACE::Tool::kAddableTitle:
      {
        Node* text = new TextGenerator();

        new NodeAddCommand(graph,
                           text,
                           command);

        new NodeEdgeAddCommand(text->output(), clip->texture_input(), command);
        break;
      }
      case OLIVE_NAMESPACE::Tool::kAddableBars:
      case OLIVE_NAMESPACE::Tool::kAddableTone:
        // Not implemented yet
        qWarning() << "Unimplemented add object:" << Core::instance()->GetSelectedAddableObject();
        break;
      case OLIVE_NAMESPACE::Tool::kAddableCount:
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
  bool snapped = parent()->SnapPoint(snap_points_, &movement);

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

OLIVE_NAMESPACE_EXIT
