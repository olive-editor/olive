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

#include "projectexplorerlistviewbase.h"

#include <QMouseEvent>

OLIVE_NAMESPACE_ENTER

ProjectExplorerListViewBase::ProjectExplorerListViewBase(QWidget *parent) :
  QListView(parent)
{
  // FIXME Is this necessary?
  setMovement(QListView::Free);

  // Set selection mode (allows multiple item selection)
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  // Set resize mode
  setResizeMode(QListView::Adjust);

  // Set widget to emit a signal on right click
  setContextMenuPolicy(Qt::CustomContextMenu);
}

void ProjectExplorerListViewBase::mouseDoubleClickEvent(QMouseEvent *event)
{
  // Perform default double click functions
  QListView::mouseDoubleClickEvent(event);

  // QAbstractItemView already has a doubleClicked() signal, but we emit another here for double clicking empty space
  if (!indexAt(event->pos()).isValid()) {
    emit DoubleClickedEmptyArea();
  }
}

OLIVE_NAMESPACE_EXIT
