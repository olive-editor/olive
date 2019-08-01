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

#include "timelineview.h"

#include "timelineviewclipitem.h"

TimelineView::TimelineView(QWidget *parent) :
  QGraphicsView(parent)
{
  setScene(&scene_);
  setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  setDragMode(RubberBandDrag);
}

void TimelineView::AddClip(ClipBlock *clip)
{
  TimelineViewClipItem* clip_item = new TimelineViewClipItem();
  clip_item->SetClip(clip);
  scene_.addItem(clip_item);

  // FIXME: Test code only
  double framerate = timebase_.ToDouble();
  double clip_item_x = clip->in().ToDouble() / framerate;
  clip_item->setRect(clip_item_x, 0, clip->out().ToDouble() / framerate - clip_item_x - 1, 100);
  // End test code
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;
}

void TimelineView::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
}

void TimelineView::Clear()
{
  scene_.clear();
}
