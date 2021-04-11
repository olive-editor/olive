/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

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
  // Cache here so if the index becomes invalid after the base call, we still know the truth
  bool item_at_location = indexAt(event->pos()).isValid();

  // Perform default double click functions
  QListView::mouseDoubleClickEvent(event);

  // QAbstractItemView already has a doubleClicked() signal, but we emit another here for double clicking empty space
  if (!item_at_location) {
    emit DoubleClickedEmptyArea();
  }
}

}
