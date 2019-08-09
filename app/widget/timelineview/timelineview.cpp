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
  playhead_(0),
  dragging_(false)
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
  clip_items_.append(clip_item);

  // Add item to graphics scene
  scene_.addItem(clip_item);
}

void TimelineView::SetScale(const double &scale)
{
  scale_ = scale;

  foreach (TimelineViewClipItem* item, clip_items_) {
    item->SetScale(scale_);
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
  QGraphicsView::mousePressEvent(event);
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  QGraphicsView::mouseMoveEvent(event);

  if (!dragging_) {
    // Let's see if there's anything selected to drag
    if (itemAt(event->pos()) != nullptr) {
      QList<QGraphicsItem*> selected_items = scene_.selectedItems();

      foreach (QGraphicsItem* item, selected_items) {
        TimelineViewGhostItem* ghost = new TimelineViewGhostItem();
        TimelineViewClipItem* clip_item = static_cast<TimelineViewClipItem*>(item);

        ClipBlock* clip = clip_item->clip();

        ghost->SetIn(clip->in());
        ghost->SetOut(clip->out());
        ghost->SetScale(scale_);

        ghost->setPos(clip_item->pos());

        ghost_items_.append(ghost);

        scene_.addItem(ghost);
      }

      drag_start_ = event->pos();
    }

    dragging_ = true;
  } else if (!ghost_items_.isEmpty()) {
    QPoint movement = event->pos() - drag_start_;

    rational time_movement = ScreenToTime(movement.x());

    // Validate movement
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      rational validator = ghost->In() + time_movement;
      if (validator < 0) {
        time_movement = rational(0) - ghost->In();
      }
    }

    // Perform movement
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      ghost->SetInAdjustment(time_movement);
      ghost->SetOutAdjustment(time_movement);
    }
  }
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  QGraphicsView::mouseReleaseEvent(event);

  ClearGhosts();

  dragging_ = false;
}

void TimelineView::dragEnterEvent(QDragEnterEvent *event)
{
  QStringList mime_formats = event->mimeData()->formats();

  // Listen for MIME data from a ProjectViewModel
  if (mime_formats.contains("application/x-oliveprojectitemdata")) {
    // FIXME: Implement audio insertion

    // Data is drag/drop data from a ProjectViewModel
    QByteArray model_data = event->mimeData()->data("application/x-oliveprojectitemdata");

    // Use QDataStream to deserialize the data
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Variables to deserialize into
    quintptr item_ptr;
    int r;

    while (!stream.atEnd()) {
      stream >> r >> item_ptr;

      // Get Item object
      Item* item = reinterpret_cast<Item*>(item_ptr);

      // Check if Item is Footage
      if (item->type() == Item::kFootage) {
        // If the Item is Footage, we can create a Ghost from it
        Footage* footage = static_cast<Footage*>(item);

        StreamPtr stream = footage->stream(0);

        TimelineViewGhostItem* ghost = new TimelineViewGhostItem();

        rational footage_duration(stream->timebase().numerator() * stream->duration(),
                                  stream->timebase().denominator());

        ghost->SetIn(0);
        ghost->SetOut(footage_duration);
        ghost->SetScale(scale_);
        ghost->SetStream(stream);

        ghost_items_.append(ghost);

        scene_.addItem(ghost);
      }
    }

    event->accept();
  } else {
    // FIXME: Implement dropping from file
    event->ignore();
  }
}

void TimelineView::dragMoveEvent(QDragMoveEvent *event)
{
  if (ghost_items_.isEmpty()) {
    event->ignore();
  } else {
    event->accept();
  }
}

void TimelineView::dragLeaveEvent(QDragLeaveEvent *event)
{
  if (ghost_items_.isEmpty()) {
    event->ignore();
  } else {
    ClearGhosts();

    event->accept();
  }
}

void TimelineView::dropEvent(QDropEvent *event)
{
  if (ghost_items_.isEmpty()) {
    event->ignore();
  } else {
    ClearGhosts();

    event->accept();
  }
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
