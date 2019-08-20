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
#include <QScrollBar>
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

  connect(&scene_, SIGNAL(changed(const QList<QRectF>&)), this, SLOT(UpdateSceneRect()));

  // Create playhead line and ensure it's always on top
  playhead_line_ = new TimelineViewPlayheadItem();
  playhead_line_->setZValue(100);

  scene_.addItem(playhead_line_);

  // Set default scale
  SetScale(1.0);
}

void TimelineView::AddBlock(Block *block, int track)
{
  switch (block->type()) {
  case Block::kClip:
  {
    TimelineViewClipItem* clip_item = new TimelineViewClipItem();
    ClipBlock* clip = static_cast<ClipBlock*>(block);

    // Set up clip with view parameters (clip item will automatically size its rect accordingly)
    clip_item->SetClip(clip);
    clip_item->SetY(GetTrackY(track));
    clip_item->SetHeight(GetTrackHeight(track));
    clip_item->SetScale(scale_);
    clip_item->SetTrack(track);

    // Add to list of clip items that can be iterated through
    clip_items_.insert(clip, clip_item);

    // Add item to graphics scene
    scene_.addItem(clip_item);

    connect(clip, SIGNAL(Refreshed()), this, SLOT(BlockChanged()));
    break;
  }
  case Block::kGap:
  {
    clip_items_.insert(block, nullptr);
    break;
  }
  case Block::kEnd:
    // Do nothing
    break;
  }


}

void TimelineView::RemoveBlock(Block *block)
{
  delete clip_items_[block];

  clip_items_.remove(block);
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;

  QMapIterator<Block*, TimelineViewRect*> iterator(clip_items_);

  while (iterator.hasNext()) {
    iterator.next();

    if (iterator.value() != nullptr) {
      iterator.value()->SetScale(scale_);
    }
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
  QMapIterator<Block*, TimelineViewRect*> iterator(clip_items_);

  while (iterator.hasNext()) {
    iterator.next();

    if (iterator.value() != nullptr) {
      delete iterator.value();
    }
  }

  clip_items_.clear();
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

void TimelineView::resizeEvent(QResizeEvent *event)
{
  QGraphicsView::resizeEvent(event);

  UpdateSceneRect();
}

int TimelineView::GetTrackY(int track_index)
{
  int y = 0;

  for (int i=0;i<track_index;i++) {
    y += GetTrackHeight(i);
  }

  return y;
}

int TimelineView::GetTrackHeight(int track_index)
{
  // FIXME: Make this adjustable
  Q_UNUSED(track_index)

  return fontMetrics().height() * 3;
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

rational TimelineView::SceneToTime(const double &x)
{
  // Adjust screen point by scale and timebase
  int scaled_x_mvmt = qRound(x / scale_ / timebase_dbl_);

  // Return a time in the timebase
  return rational(scaled_x_mvmt * timebase_.numerator(), timebase_.denominator());
}

int TimelineView::SceneToTrack(const double &y)
{
  int track = -1;
  int heights = 0;

  do {
    track++;
    heights += GetTrackHeight(track);
  } while (y > heights);

  return track;
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
  TimelineViewRect* rect = clip_items_[static_cast<Block*>(sender())];

  if (rect != nullptr) {
    rect->UpdateRect();
  }
}

void TimelineView::UpdateSceneRect()
{
  QRectF bounding_rect = scene_.itemsBoundingRect();

  // Ensure the scene left and top are always 0
  bounding_rect.setTopLeft(QPointF(0, 0));

  // Ensure the scene height is always AT LEAST the height of the view
  int minimum_height = height() - horizontalScrollBar()->height() - 2;
  if (bounding_rect.height() < minimum_height) {
    bounding_rect.setHeight(minimum_height);
  }

  // Ensure playhead is the correct height
  playhead_line_->UpdateRect();

  // If the scene is already this rect, do nothing
  if (scene_.sceneRect() == bounding_rect) {
    return;
  }

  scene_.setSceneRect(bounding_rect);
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

QPointF TimelineView::Tool::GetScenePos(const QPoint &screen_pos)
{
  return parent()->mapToScene(screen_pos);
}

QGraphicsItem *TimelineView::Tool::GetItemAtScenePos(const QPointF &scene_pos)
{
  return parent()->scene_.itemAt(scene_pos, parent()->transform());
}
