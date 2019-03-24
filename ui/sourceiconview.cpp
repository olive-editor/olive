/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "sourceiconview.h"

#include <QMimeData>

#include "panels/project.h"
#include "project/media.h"
#include "project/sourcescommon.h"
#include "global/debug.h"

SourceIconView::SourceIconView(SourcesCommon &commons) :
  commons_(commons)
{
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setResizeMode(QListView::Adjust);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(item_click(const QModelIndex&)));
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
}

void SourceIconView::show_context_menu() {
  commons_.show_context_menu(this, selectedIndexes());
}

void SourceIconView::item_click(const QModelIndex& index) {
  if (selectedIndexes().size() == 1 && index.column() == 0) {
    commons_.item_click(project_parent->item_to_media(index), index);
  }
}

void SourceIconView::mousePressEvent(QMouseEvent* event) {
  commons_.mousePressEvent(event);
  if (!indexAt(event->pos()).isValid()) selectionModel()->clear();
  QListView::mousePressEvent(event);
}

void SourceIconView::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  } else {
    QListView::dragEnterEvent(event);
  }
}

void SourceIconView::dragMoveEvent(QDragMoveEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  } else {
    QListView::dragMoveEvent(event);
  }
}

void SourceIconView::dropEvent(QDropEvent* event) {
  QModelIndex drop_item = indexAt(event->pos());
  if (!drop_item.isValid()) drop_item = rootIndex();
  commons_.dropEvent(this, event, drop_item, selectedIndexes());
}

void SourceIconView::mouseDoubleClickEvent(QMouseEvent *) {
  if (selectedIndexes().size() == 1) {
    Media* m = project_parent->item_to_media(selectedIndexes().at(0));
    if (m->get_type() == MEDIA_TYPE_FOLDER) {
      setRootIndex(selectedIndexes().at(0));
      emit changed_root();
      return;
    }
  }

  // Double click was not a folder, so we perform the default behavior (sending the double click to SourcesCommon)
  commons_.mouseDoubleClickEvent(selectedIndexes());
}
