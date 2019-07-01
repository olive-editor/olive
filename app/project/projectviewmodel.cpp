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

#include "projectviewmodel.h"

ProjectViewModel::ProjectViewModel(QObject *parent) :
  QAbstractItemModel(parent),
  project_(nullptr)
{
  // TODO make this configurable
  columns_.append(kName);
  columns_.append(kDuration);
  columns_.append(kRate);
}

Project *ProjectViewModel::project()
{
  return project_;
}

void ProjectViewModel::set_project(Project *p)
{
  beginResetModel();

  project_ = p;

  endResetModel();
}

QModelIndex ProjectViewModel::index(int row, int column, const QModelIndex &parent) const
{
  // I'm actually not 100% sure what this does, but it seems logical and was in the earlier code
  if (!hasIndex(row, column, parent)) {
    return QModelIndex();
  }

  // Get the parent object (project root if the index is invalid)
  Item* item_parent = (parent.isValid()) ? static_cast<Item*>(parent.internalPointer()) : project_->root();

  // Return an index to this object
  return createIndex(row, column, item_parent->child(row));
}

QModelIndex ProjectViewModel::parent(const QModelIndex &child) const
{
  // Get the Item object from the index
  Item* item = static_cast<Item*>(child.internalPointer());

  // Get Item's parent object
  Item* parent = item->parent();

  // If the parent is the root, return an empty index
  if (parent == project_->root()) {
    return QModelIndex();
  }

  // Otherwise return a true index to its parent

  // Find parent's index within its own parent
  // (TODO: this model should handle sorting, which means it'll have to "know" the indices)
  int child_index = -1;

  Item* double_parent = parent->parent();
  for (int i=0;i<double_parent->child_count();i++) {
    if (double_parent->child(i) == parent) {
      child_index = i;
      break;
    }
  }

  // Make sure the index is valid (there's no reason it shouldn't be)
  Q_ASSERT(child_index > -1);

  // Return an index to the parent
  return createIndex(child_index, 0, parent);
}

int ProjectViewModel::rowCount(const QModelIndex &parent) const
{
  // If there's no project, there are obviously no items to show
  if (project_ == nullptr) {
    return 0;
  }

  // If the index is the root, return the root child count
  if (parent == QModelIndex()) {
    return project_->root()->child_count();
  }

  // Otherwise, the index must contain a valid pointer, so we just return its child count
  return static_cast<Item*>(parent.internalPointer())->child_count();
}

int ProjectViewModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent)

  // Not strictly necessary, but a decent visual cue that there's no project currently active
  if (project_ == nullptr) {
    return 0;
  }

  return columns_.size();
}

QVariant ProjectViewModel::data(const QModelIndex &index, int role) const
{
  Item* internal_item = static_cast<Item*>(index.internalPointer());

  switch (role) {
  case Qt::DisplayRole:
  {
    // Standard text role
    ColumnType column_type = columns_.at(index.column());

    switch (column_type) {
    case kName:
      return internal_item->name();
    case kDuration:
      // FIXME Return actual information
      return "00:00:00;00";
    case kRate:
      // FIXME Return actual information
      return "29.97 FPS";
    }
  }
    break;
  case Qt::DecorationRole:
    // If this is the first column, return the Item's icon
    if (index.column() == 0) {
      return internal_item->icon();
    }
    break;
  }

  return QVariant();
}

QVariant ProjectViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  // Check if we need text data (DisplayRole) and orientation is horizontal
  // FIXME I'm not 100% sure what happens if the orientation is vertical/if that check is necessary
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    ColumnType column_type = columns_.at(section);

    // Return the name based on the column's current type
    switch (column_type) {
    case kName:
      return tr("Name");
    case kDuration:
      return tr("Duration");
    case kRate:
      return tr("Rate");
    }
  }

  return QAbstractItemModel::headerData(section, orientation, role);
}

bool ProjectViewModel::hasChildren(const QModelIndex &parent) const
{
  // Check if this is a valid index
  if (parent.isValid()) {
    Item* item = static_cast<Item*>(parent.internalPointer());

    // Check if this item is a kFolder type
    // If it's a folder, we always return TRUE in order to always show the "expand triangle" icon,
    // even when there are no "physical" children
    if (item->type() == Item::kFolder) {
      return true;
    }
  }

  // Otherwise, return default behavior
  return QAbstractItemModel::hasChildren(parent);
}
