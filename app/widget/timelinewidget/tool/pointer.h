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

#ifndef POINTERTIMELINETOOL_H
#define POINTERTIMELINETOOL_H

#include "tool.h"

namespace olive {

class PointerTool : public TimelineTool
{
public:
  PointerTool(TimelineWidget* parent);

  virtual void MousePress(TimelineViewMouseEvent *event) override;
  virtual void MouseMove(TimelineViewMouseEvent *event) override;
  virtual void MouseRelease(TimelineViewMouseEvent *event) override;

  virtual void HoverMove(TimelineViewMouseEvent *event) override;

protected:
  virtual void FinishDrag(TimelineViewMouseEvent *event);

  virtual void InitiateDrag(Block* clicked_item,
                            Timeline::MovementMode trim_mode);

  TimelineViewGhostItem* AddGhostFromBlock(Block *block, const TrackReference& track, Timeline::MovementMode mode, bool check_if_exists = false);

  TimelineViewGhostItem* AddGhostFromNull(const rational& in, const rational& out, const TrackReference& track, Timeline::MovementMode mode);

  /**
   * @brief Validates Ghosts that are getting their in points trimmed
   *
   * Assumes ghost->data() is a Block. Ensures no Ghost's in point becomes a negative timecode. Also ensures no
   * Ghost's length becomes 0 or negative.
   */
  rational ValidateInTrimming(rational movement);

  /**
   * @brief Validates Ghosts that are getting their out points trimmed
   *
   * Assumes ghost->data() is a Block. Ensures no Ghost's in point becomes a negative timecode. Also ensures no
   * Ghost's length becomes 0 or negative.
   */
  rational ValidateOutTrimming(rational movement);

  virtual void ProcessDrag(const TimelineCoordinate &mouse_pos);

  void InitiateDragInternal(Block* clicked_item,
                            Timeline::MovementMode trim_mode,
                            bool dont_roll_trims,
                            bool allow_nongap_rolling, bool slide_instead_of_moving);

  const Timeline::MovementMode& drag_movement_mode() const
  {
    return drag_movement_mode_;
  }

  void SetMovementAllowed(bool e)
  {
    movement_allowed_ = e;
  }

  void SetTrimmingAllowed(bool e)
  {
    trimming_allowed_ = e;
  }

  void SetTrackMovementAllowed(bool e)
  {
    track_movement_allowed_ = e;
  }

  void SetGapTrimmingAllowed(bool e)
  {
    gap_trimming_allowed_ = e;
  }

private:
  Timeline::MovementMode IsCursorInTrimHandle(Block* block, qreal cursor_x);

  void AddGhostInternal(TimelineViewGhostItem* ghost, Timeline::MovementMode mode);

  bool IsClipTrimmable(Block* clip,
                       const QVector<Block*> &items,
                       const Timeline::MovementMode& mode);

  void ProcessGhostsForSliding();

  void ProcessGhostsForRolling();

  bool AddMovingTransitionsToClipGhost(Block *block, const TrackReference &track, Timeline::MovementMode movement, const QVector<Block*> &selected_items);

  bool movement_allowed_;
  bool trimming_allowed_;
  bool track_movement_allowed_;
  bool gap_trimming_allowed_;
  bool can_rubberband_select_;
  bool rubberband_selecting_;

  Track::Type drag_track_type_;
  Timeline::MovementMode drag_movement_mode_;

  Block* clicked_item_;

  QPoint drag_global_start_;

};

}

#endif // POINTERTIMELINETOOL_H
