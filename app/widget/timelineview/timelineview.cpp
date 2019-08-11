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

#include <QDebug>
#include <QMimeData>
#include <QMouseEvent>
#include <QtMath>
#include <QPen>

#include "node/input/media/media.h"
#include "project/item/footage/footage.h"

TimelineView::TimelineView(QWidget *parent) :
  QGraphicsView(parent),
  pointer_tool_(this),
  import_tool_(this),
  playhead_(0)
{
  setScene(&scene_);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  setDragMode(RubberBandDrag);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

  // Create playhead line and ensure it's always on top
  playhead_line_ = new TimelineViewPlayheadItem();
  playhead_line_->setZValue(100);

  scene_.addItem(playhead_line_);

  // Set default scale
  SetScale(1.0);
}

void TimelineView::AddClip(ClipBlock *clip)
{
  TimelineViewClipItem* clip_item = new TimelineViewClipItem();

  // Set up clip with view parameters (clip item will automatically size its rect accordingly)
  clip_item->SetClip(clip);
  clip_item->SetScale(scale_);

  // Add to list of clip items that can be iterated through
  clip_items_.insert(clip, clip_item);

  // Add item to graphics scene
  scene_.addItem(clip_item);

  connect(clip, SIGNAL(Refreshed()), this, SLOT(BlockChanged()));
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;

  QMapIterator<Block*, TimelineViewRect*> iterator(clip_items_);

  while (iterator.hasNext()) {
    iterator.value()->SetScale(scale_);
  }

  playhead_line_->SetScale(scale_);
}

void TimelineView::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;
  timebase_dbl_ = timebase_.toDouble();

  playhead_line_->SetTimebase(timebase_);
}

void TimelineView::Clear()
{
  scene_.removeItem(playhead_line_);

  scene_.clear();
  clip_items_.clear();

  scene_.addItem(playhead_line_);
}

void TimelineView::SetTime(const int64_t time)
{
  playhead_ = time;

  playhead_line_->SetPlayhead(playhead_);
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
  pointer_tool_.MousePress(event);
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  pointer_tool_.MouseMove(event);
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  pointer_tool_.MouseRelease(event);
}

void TimelineView::dragEnterEvent(QDragEnterEvent *event)
{
  import_tool_.DragEnter(event);
}

void TimelineView::dragMoveEvent(QDragMoveEvent *event)
{
  import_tool_.DragMove(event);
}

void TimelineView::dragLeaveEvent(QDragLeaveEvent *event)
{
  import_tool_.DragLeave(event);
}

void TimelineView::dropEvent(QDropEvent *event)
{
  import_tool_.DragDrop(event);
}

void TimelineView::AddGhost(TimelineViewGhostItem *ghost)
{
  ghost->SetScale(scale_);
  ghost_items_.append(ghost);
  scene_.addItem(ghost);
}

bool TimelineView::HasGhosts()
{
  return !ghost_items_.isEmpty();
}

rational TimelineView::ScreenToTime(const int &x)
{
  // Adjust screen point by scale and timebase
  int scaled_x_mvmt = qRound(x / scale_ / timebase_dbl_);

  // Return a time in the timebase
  return rational(scaled_x_mvmt * timebase_.numerator(), timebase_.denominator());
}

void TimelineView::ClearGhosts()
{
  if (!ghost_items_.isEmpty()) {
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      delete ghost;
    }

    ghost_items_.clear();
  }
}

void TimelineView::BlockChanged()
{
  clip_items_[static_cast<Block*>(sender())]->UpdateRect();
}

TimelineView::Tool::Tool(TimelineView *parent) :
  dragging_(false),
  parent_(parent)
{
}

TimelineView::Tool::~Tool(){}

void TimelineView::Tool::MousePress(QMouseEvent *){}

void TimelineView::Tool::MouseMove(QMouseEvent *){}

void TimelineView::Tool::MouseRelease(QMouseEvent *){}

void TimelineView::Tool::DragEnter(QDragEnterEvent *){}

void TimelineView::Tool::DragMove(QDragMoveEvent *){}

void TimelineView::Tool::DragLeave(QDragLeaveEvent *){}

void TimelineView::Tool::DragDrop(QDropEvent *){}

TimelineView *TimelineView::Tool::parent()
{
  return parent_;
}
