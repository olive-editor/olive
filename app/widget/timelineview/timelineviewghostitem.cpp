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
  in_adj_(0),
  out_adj_(0),
  track_adj_(0),
  stream_(nullptr),
  mode_(kNone)
{
  setBrush(Qt::NoBrush);
  setPen(QPen(Qt::yellow, 2)); // FIXME: Make customizable via CSS
}

TimelineViewGhostItem *TimelineViewGhostItem::FromClip(TimelineViewClipItem *clip_item)
{
  TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

  ghost->SetY(clip_item->Y());
  ghost->SetHeight(clip_item->Height());
  ghost->SetTrack(clip_item->Track());
  ghost->setPos(clip_item->pos());

  ClipBlock* clip = clip_item->clip();

  ghost->SetIn(clip->in());
  ghost->SetOut(clip->out());
  ghost->setData(0, Node::PtrToValue(clip));

  return ghost;
}

const rational &TimelineViewGhostItem::In() const
{
  return in_;
}

const rational &TimelineViewGhostItem::Out() const
{
  return out_;
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

const rational &TimelineViewGhostItem::InAdjustment() const
{
  return in_adj_;
}

const rational &TimelineViewGhostItem::OutAdjustment() const
{
  return out_adj_;
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

int TimelineViewGhostItem::GetAdjustedTrack() const
{
  return track_ + track_adj_;
}

const TimelineViewGhostItem::Mode &TimelineViewGhostItem::mode() const
{
  return mode_;
}

void TimelineViewGhostItem::SetMode(const TimelineViewGhostItem::Mode &mode)
{
  mode_ = mode;
}

void TimelineViewGhostItem::UpdateRect()
{
  rational length = GetAdjustedOut() - GetAdjustedIn();

  setRect(0, y_, TimeToScreenCoord(length), height_ - 1);

  setPos(TimeToScreenCoord(GetAdjustedIn()), 0);
}
