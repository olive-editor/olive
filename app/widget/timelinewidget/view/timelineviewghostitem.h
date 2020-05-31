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

#ifndef TIMELINEVIEWGHOSTITEM_H
#define TIMELINEVIEWGHOSTITEM_H

#include <QVariant>

#include "project/item/footage/footage.h"
#include "timeline/timelinecommon.h"
#include "timelineviewblockitem.h"
#include "timelineviewrect.h"

OLIVE_NAMESPACE_ENTER
/**
 * @brief A graphical representation of changes the user is making before they apply it
 */
class TimelineViewGhostItem : public TimelineViewRect
{
public:
  enum DataType {
    kAttachedBlock,
    kReferenceBlock,
    kAttachedFootage
  };

  TimelineViewGhostItem(QGraphicsItem* parent = nullptr);

  static TimelineViewGhostItem* FromBlock(Block *block, const TrackReference &track, int y, int height);

  bool CanHaveZeroLength() const;

  bool CanMoveTracks() const;
  void SetCanMoveTracks(bool e);

  void SetInvisible(bool invisible);

  const rational& In() const;
  const rational& Out() const;
  const rational& MediaIn() const;

  rational Length() const;
  rational AdjustedLength() const;

  void SetIn(const rational& in);
  void SetOut(const rational& out);
  void SetMediaIn(const rational& media_in);

  void SetInAdjustment(const rational& in_adj);
  void SetOutAdjustment(const rational& out_adj);
  void SetTrackAdjustment(const int& track_adj);
  void SetMediaInAdjustment(const rational& media_in_adj);

  const rational& InAdjustment() const;
  const rational& OutAdjustment() const;
  const rational& MediaInAdjustment() const;
  const int& TrackAdjustment() const;

  rational GetAdjustedIn() const;
  rational GetAdjustedOut() const;
  rational GetAdjustedMediaIn() const;
  TrackReference GetAdjustedTrack() const;

  const Timeline::MovementMode& mode() const;
  void SetMode(const Timeline::MovementMode& mode);

  bool HasBeenAdjusted() const;

  virtual void UpdateRect() override;

protected:

private:
  rational in_;
  rational out_;
  rational media_in_;

  rational in_adj_;
  rational out_adj_;
  rational media_in_adj_;

  int track_adj_;

  StreamPtr stream_;

  Timeline::MovementMode mode_;

  bool can_have_zero_length_;
  bool can_move_tracks_;
};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEVIEWGHOSTITEM_H
