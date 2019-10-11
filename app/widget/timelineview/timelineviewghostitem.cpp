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

#include "timelineviewghostitem.h"

#include <QPainter>

TimelineViewGhostItem::TimelineViewGhostItem(QGraphicsItem *parent) :
  TimelineViewRect(parent),
  track_adj_(0),
  stream_(nullptr),
  mode_(olive::timeline::kNone),
  can_have_zero_length_(true)
{
  SetInvisible(false);
}

TimelineViewGhostItem *TimelineViewGhostItem::FromBlock(Block *block, const TrackReference& track, int y, int height)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetIn(block->in());
  ghost->SetOut(block->out());
  ghost->SetMediaIn(block->media_in());
  ghost->SetTrack(track);
  ghost->SetY(y);
  ghost->SetHeight(height);
  ghost->setData(kAttachedBlock, Node::PtrToValue(block));

  if (block->type() == Block::kClip) {
    ghost->can_have_zero_length_ = false;
  }

  return ghost;
}

bool TimelineViewGhostItem::CanHaveZeroLength()
{
  return can_have_zero_length_;
}

void TimelineViewGhostItem::SetInvisible(bool invisible)
{
  setBrush(Qt::NoBrush);

  if (invisible) {
    setPen(Qt::NoPen);
  } else {
    setPen(QPen(Qt::yellow, 2)); // FIXME: Make customizable via CSS
  }
}

const rational &TimelineViewGhostItem::In() const
{
  return in_;
}

const rational &TimelineViewGhostItem::Out() const
{
  return out_;
}

const rational &TimelineViewGhostItem::MediaIn() const
{
  return media_in_;
}

rational TimelineViewGhostItem::Length() const
{
  return out_ - in_;
}

rational TimelineViewGhostItem::AdjustedLength() const
{
  return GetAdjustedOut() - GetAdjustedIn();
}

void TimelineViewGhostItem::SetIn(const rational &in)
{
  in_ = in;

  UpdateRect();
}

void TimelineViewGhostItem::SetOut(const rational &out)
{
  out_ = out;

  UpdateRect();
}

void TimelineViewGhostItem::SetMediaIn(const rational &media_in)
{
  media_in_ = media_in;
}

void TimelineViewGhostItem::SetInAdjustment(const rational &in_adj)
{
  in_adj_ = in_adj;

  UpdateRect();
}

void TimelineViewGhostItem::SetOutAdjustment(const rational &out_adj)
{
  out_adj_ = out_adj;

  UpdateRect();
}

void TimelineViewGhostItem::SetTrackAdjustment(const int &track_adj)
{
  track_adj_ = track_adj;
}

void TimelineViewGhostItem::SetMediaInAdjustment(const rational &media_in_adj)
{
  media_in_adj_ = media_in_adj;
}

const rational &TimelineViewGhostItem::InAdjustment() const
{
  return in_adj_;
}

const rational &TimelineViewGhostItem::OutAdjustment() const
{
  return out_adj_;
}

const rational &TimelineViewGhostItem::MediaInAdjustment() const
{
  return media_in_adj_;
}

const int &TimelineViewGhostItem::TrackAdjustment() const
{
  return track_adj_;
}

rational TimelineViewGhostItem::GetAdjustedIn() const
{
  return in_ + in_adj_;
}

rational TimelineViewGhostItem::GetAdjustedOut() const
{
  return out_ + out_adj_;
}

rational TimelineViewGhostItem::GetAdjustedMediaIn() const
{
  return media_in_ + media_in_adj_;
}

TrackReference TimelineViewGhostItem::GetAdjustedTrack() const
{
  return track_ + track_adj_;
}

const olive::timeline::MovementMode &TimelineViewGhostItem::mode() const
{
  return mode_;
}

void TimelineViewGhostItem::SetMode(const olive::timeline::MovementMode &mode)
{
  mode_ = mode;
}

void TimelineViewGhostItem::UpdateRect()
{
  rational length = GetAdjustedOut() - GetAdjustedIn();

  setRect(0, y_, TimeToScene(length), height_ - 1);

  setPos(TimeToScene(GetAdjustedIn()), 0);
}
