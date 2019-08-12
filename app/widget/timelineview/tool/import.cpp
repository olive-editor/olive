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

#include "widget/timelineview/timelineview.h"

#include <QMimeData>

#include "node/input/media/media.h"

TimelineView::ImportTool::ImportTool(TimelineView *parent) :
  Tool(parent)
{
}

void TimelineView::ImportTool::DragEnter(QDragEnterEvent *event)
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
        ghost->SetStream(stream);

        parent()->AddGhost(ghost);
      }
    }

    event->accept();
  } else {
    // FIXME: Implement dropping from file
    event->ignore();
  }
}

void TimelineView::ImportTool::DragMove(QDragMoveEvent *event)
{
  if (parent()->HasGhosts()) {
    // Move ghosts to the mouse cursor
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      rational time = parent()->ScreenToTime(event->pos().x());

      ghost->SetOut(ghost->Out() - ghost->In() + time);
      ghost->SetIn(time);
    }

    event->accept();
  } else {
    event->ignore();
  }
}

void TimelineView::ImportTool::DragLeave(QDragLeaveEvent *event)
{
  if (parent()->HasGhosts()) {
    parent()->ClearGhosts();

    event->accept();
  } else {
    event->ignore();
  }
}

void TimelineView::ImportTool::DragDrop(QDropEvent *event)
{
  if (parent()->HasGhosts()) {
    foreach (TimelineViewGhostItem* ghost, parent()->ghost_items_) {
      ClipBlock* clip = new ClipBlock();
      MediaInput* media = new MediaInput();

      clip->set_length(ghost->Out() - ghost->In());
      media->SetFootage(ghost->stream()->footage());

      NodeParam::ConnectEdge(media->texture_output(), clip->texture_input());

      // FIXME: If this doesn't have a TimelineOutput node attached, this is a memory leak. Maybe switching nodes to
      //        shared ptrs would be a better idea.
      emit parent()->RequestPlaceBlock(clip, ghost->In());
    }

    parent()->ClearGhosts();

    event->accept();
  } else {
    event->ignore();
  }
}
