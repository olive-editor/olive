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

#include <QMouseEvent>
#include <QtMath>

TimelineView::TimelineView(QWidget *parent) :
  QGraphicsView(parent)
{
  setScene(&scene_);
  setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  setDragMode(RubberBandDrag);

  // Set default scale
  SetScale(1.0);
}

void TimelineView::AddClip(ClipBlock *clip)
{
  TimelineViewClipItem* clip_item = new TimelineViewClipItem();

  // Set up clip with view parameters (clip item will automatically size its rect accordingly)
  clip_item->SetClip(clip);
  clip_item->SetTimebase(timebase_);
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
}

void TimelineView::SetTimebase(const rational &timebase)
{
  timebase_ = timebase;

  foreach (TimelineViewClipItem* item, clip_items_) {
    item->SetTimebase(timebase_);
  }
}

void TimelineView::Clear()
{
  scene_.clear();
  clip_items_.clear();
  // FIXME: Does QGraphicsScene take ownership of these items or do I have to delete them manually after the scene
  //        clear?
}

void TimelineView::mousePressEvent(QMouseEvent *event)
{
  QGraphicsView::mousePressEvent(event);

  if (itemAt(event->pos()) != nullptr) {
    QList<QGraphicsItem*> selected_items = scene_.selectedItems();

    foreach (QGraphicsItem* item, selected_items) {
      TimelineViewGhostItem* ghost = new TimelineViewGhostItem();
      TimelineViewClipItem* clip_item = static_cast<TimelineViewClipItem*>(item);

      ghost->setRect(clip_item->rect());

      ghost_items_.append(ghost);

      scene_.addItem(ghost);
    }
  }
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
{
  QGraphicsView::mouseMoveEvent(event);

  if (!ghost_items_.isEmpty()) {
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      QPointF scene_pos = mapToScene(event->pos());

      ghost->setX(qMax(0.0, scene_pos.x()));
    }
  }
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
{
  QGraphicsView::mouseReleaseEvent(event);

  if (!ghost_items_.isEmpty()) {
    foreach (TimelineViewGhostItem* ghost, ghost_items_) {
      delete ghost;
    }

    ghost_items_.clear();
  }
}
