/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "node/block/clip/clip.h"
#include "node/block/transition/transition.h"
#include "node/output/track/track.h"
#include "node/project/footage/footage.h"
#include "timeline/timelinecommon.h"

namespace olive {
/**
 * @brief A graphical representation of changes the user is making before they apply it
 */
class TimelineViewGhostItem
{
public:
  enum DataType {
    kAttachedBlock,
    kReferenceBlock,
    kAttachedFootage,
    kGhostIsSliding,
    kTrimIsARollEdit,
    kTrimShouldBeIgnored
  };

  struct AttachedFootage {
    ViewerOutput* footage;
    QString output;
  };

  TimelineViewGhostItem() :
    track_adj_(0),
    mode_(Timeline::kNone),
    can_have_zero_length_(true),
    can_move_tracks_(true),
    invisible_(false)
  {
  }

  static TimelineViewGhostItem* FromBlock(Block *block)
  {
    TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

    ghost->SetIn(block->in());
    ghost->SetOut(block->out());
    ghost->SetMediaIn(block->media_in());
    ghost->SetTrack(block->track()->ToReference());
    ghost->SetData(kAttachedBlock, Node::PtrToValue(block));

    if (dynamic_cast<ClipBlock*>(block)) {
      ghost->can_have_zero_length_ = false;
    } else if (dynamic_cast<TransitionBlock*>(block)) {
      ghost->can_have_zero_length_ = false;
      ghost->SetCanMoveTracks(false);
    }

    return ghost;
  }

  bool CanHaveZeroLength() const
  {
    return can_have_zero_length_;
  }

  bool GetCanMoveTracks() const
  {
    return can_move_tracks_;
  }

  void SetCanMoveTracks(bool e)
  {
    can_move_tracks_ = e;
  }

  const rational& GetIn() const
  {
    return in_;
  }

  const rational& GetOut() const
  {
    return out_;
  }

  const rational& GetMediaIn() const
  {
    return media_in_;
  }

  rational GetLength() const
  {
    return out_ - in_;
  }

  rational GetAdjustedLength() const
  {
    return GetAdjustedOut() - GetAdjustedIn();
  }

  void SetIn(const rational& in)
  {
    in_ = in;
  }

  void SetOut(const rational& out)
  {
    out_ = out;
  }

  void SetMediaIn(const rational& media_in)
  {
    media_in_ = media_in;
  }

  void SetInAdjustment(const rational& in_adj)
  {
    in_adj_ = in_adj;
  }

  void SetOutAdjustment(const rational& out_adj)
  {
    out_adj_ = out_adj;
  }

  void SetTrackAdjustment(const int& track_adj)
  {
    track_adj_ = track_adj;
  }

  void SetMediaInAdjustment(const rational& media_in_adj)
  {
    media_in_adj_ = media_in_adj;
  }

  const rational& GetInAdjustment() const
  {
    return in_adj_;
  }

  const rational& GetOutAdjustment() const
  {
    return out_adj_;
  }

  const rational& GetMediaInAdjustment() const
  {
    return media_in_adj_;
  }

  const int& GetTrackAdjustment() const
  {
    return track_adj_;
  }

  rational GetAdjustedIn() const
  {
    return in_ + in_adj_;
  }

  rational GetAdjustedOut() const
  {
    return out_ + out_adj_;
  }

  rational GetAdjustedMediaIn() const
  {
    return media_in_ + media_in_adj_;
  }

  Track::Reference GetAdjustedTrack() const
  {
    return Track::Reference(track_.type(), track_.index() + track_adj_);
  }

  const Timeline::MovementMode& GetMode() const
  {
    return mode_;
  }

  void SetMode(const Timeline::MovementMode& mode)
  {
    mode_ = mode;
  }

  bool HasBeenAdjusted() const
  {
    return GetInAdjustment() != 0
        || GetOutAdjustment() != 0
        || GetMediaInAdjustment() != 0
        || GetTrackAdjustment() != 0;
  }

  QVariant GetData(int key) const
  {
    return data_.value(key);
  }

  void SetData(int key, const QVariant& value)
  {
    data_.insert(key, value);
  }

  const Track::Reference& GetTrack() const
  {
    return track_;
  }

  void SetTrack(const Track::Reference& track)
  {
    track_ = track;
  }

  bool IsInvisible() const
  {
    return invisible_;
  }

  void SetInvisible(bool e)
  {
    invisible_ = e;
  }

protected:

private:
  rational in_;
  rational out_;
  rational media_in_;

  rational in_adj_;
  rational out_adj_;
  rational media_in_adj_;

  int track_adj_;

  Timeline::MovementMode mode_;

  bool can_have_zero_length_;
  bool can_move_tracks_;

  Track::Reference track_;

  QHash<int, QVariant> data_;

  bool invisible_;

};

}

Q_DECLARE_METATYPE(olive::TimelineViewGhostItem::AttachedFootage)

#endif // TIMELINEVIEWGHOSTITEM_H
