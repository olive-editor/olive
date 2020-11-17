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

#include "projectviewmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QUrl>

#include "core.h"
#include "node/input/media/media.h"

OLIVE_NAMESPACE_ENTER

ProjectViewModel::ProjectViewModel(QObject *parent) :
  QAbstractItemModel(parent),
  project_(nullptr)
{
  // FIXME: make this configurable
  columns_.append(kName);
  columns_.append(kDuration);
  columns_.append(kRate);
}

Project *ProjectViewModel::project() const
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
  Item* item_parent = GetItemObjectFromIndex(parent);

  // Return an index to this object
  return createIndex(row, column, item_parent->child(row));
}

QModelIndex ProjectViewModel::parent(const QModelIndex &child) const
{
  // Get the Item object from the index
  Item* item = GetItemObjectFromIndex(child);

  // Get Item's parent object
  Item* par = item->parent();

  // If the parent is the root, return an empty index
  if (par == project_->root()) {
    return QModelIndex();
  }

  // Otherwise return a true index to its parent
  int parent_index = IndexOfChild(par);

  // Make sure the index is valid (there's no reason it shouldn't be)
  Q_ASSERT(parent_index > -1);

  // Return an index to the parent
  return createIndex(parent_index, 0, par);
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
  return GetItemObjectFromIndex(parent)->child_count();
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
  Item* internal_item = GetItemObjectFromIndex(index);

  ColumnType column_type = columns_.at(index.column());

  switch (role) {
  case Qt::DisplayRole:
  {
    // Standard text role

    switch (column_type) {
    case kName:
      return internal_item->name();
    case kDuration:
      return internal_item->duration();
    case kRate:
      return internal_item->rate();
    }
  }
    break;
  case Qt::DecorationRole:
    // If this is the first column, return the Item's icon
    if (column_type == kName) {
      return internal_item->icon();
    }
    break;
  case Qt::ToolTipRole:
    return internal_item->tooltip();
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
    Item* item = GetItemObjectFromIndex(parent);

    // Check if this item is a kFolder type
    // If it's a folder, we always return TRUE in order to always show the "expand triangle" icon,
    // even when there are no "physical" children
    if (item->CanHaveChildren()) {
      return true;
    }
  }

  // Otherwise, return default behavior
  return QAbstractItemModel::hasChildren(parent);
}

bool ProjectViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  // The name is editable
  if (index.isValid() && columns_.at(index.column()) == kName && role == Qt::EditRole) {
    Item* item = GetItemObjectFromIndex(index);

    RenameItemCommand* ric = new RenameItemCommand(this, item, value.toString());

    Core::instance()->undo_stack()->push(ric);

    return true;
  }

  return false;
}

bool ProjectViewModel::canFetchMore(const QModelIndex &parent) const
{
  // Check if this is a valid index
  if (parent.isValid()) {
    Item* item = GetItemObjectFromIndex(parent);

    // Check if this item is a kFolder type
    // If it's a folder, we always return TRUE in order to always show the "expand triangle" icon,
    // even when there are no "physical" children
    if (item->CanHaveChildren()) {
      return true;
    }
  }

  // Otherwise, return default behavior
  return QAbstractItemModel::canFetchMore(parent);
}

Qt::ItemFlags ProjectViewModel::flags(const QModelIndex &index) const
{
  if (!index.isValid()) {
    // Allow dropping files from external sources
    return Qt::ItemIsDropEnabled;
  }

  Qt::ItemFlags f = Qt::ItemIsDragEnabled | QAbstractItemModel::flags(index);

  if (GetItemObjectFromIndex(index)->CanHaveChildren()) {
    f |= Qt::ItemIsDropEnabled;
  }

  // If the column is the kName column, that means it's editable
  if (columns_.at(index.column()) == kName) {
    f |= Qt::ItemIsEditable;
  }

  return f;
}

QStringList ProjectViewModel::mimeTypes() const
{
  // Allow data from this model and a file list from external sources
  return {"application/x-oliveprojectitemdata", "text/uri-list"};
}

QMimeData *ProjectViewModel::mimeData(const QModelIndexList &indexes) const
{
  // Compliance with Qt standard
  if (indexes.isEmpty()) {
    return nullptr;
  }

  // Encode mime data for the rows/items that were dragged
  QMimeData* data = new QMimeData();

  // Use QDataStream to stream the item data into a byte array
  QByteArray encoded_data;
  QDataStream stream(&encoded_data, QIODevice::WriteOnly);

  // The indexes list includes indexes for each column which we don't use. To make sure each row only gets sent *once*,
  // we keep a list of dragged items
  QVector<void*> dragged_items;

  foreach (QModelIndex index, indexes) {
    if (index.isValid()) {
      // Check if we've dragged this item before
      if (!dragged_items.contains(index.internalPointer())) {
        // If not, add it to the stream (and also keep track of it in the vector)
        quint64 stream_flags;

        if (static_cast<Item*>(index.internalPointer())->type() == Item::kFootage) {
          stream_flags = static_cast<Footage*>(index.internalPointer())->get_enabled_stream_flags();
        } else {
          stream_flags = UINT64_MAX;
        }

        stream << stream_flags << index.row() << reinterpret_cast<quintptr>(index.internalPointer());
        dragged_items.append(index.internalPointer());
      }
    }
  }

  // Set byte array as the mime data and return the mime data
  data->setData("application/x-oliveprojectitemdata", encoded_data);

  return data;
}

bool ProjectViewModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &drop)
{
  // Default recommended checks from https://doc.qt.io/qt-5/model-view-programming.html#using-drag-and-drop-with-item-views
  if (!canDropMimeData(data, action, row, column, drop)) {
    return false;
  }

  if (action == Qt::IgnoreAction) {
    return true;
  }

  // Probe mime data for its format
  QStringList mime_formats = data->formats();

  if (mime_formats.contains("application/x-oliveprojectitemdata")) {
    // Data is drag/drop data from this model
    QByteArray model_data = data->data("application/x-oliveprojectitemdata");

    // Use QDataStream to deserialize the data
    QDataStream stream(&model_data, QIODevice::ReadOnly);

    // Get the Item object that the items were dropped on
    Item* drop_location = GetItemObjectFromIndex(drop);

    // If this is not a folder, we cannot drop these items here
    if (!drop_location->CanHaveChildren()) {
      return false;
    }

    // Variables to deserialize into
    quintptr item_ptr;
    int r;
    quint64 enabled_streams;

    // Loop through all data
    QUndoCommand* move_command = new QUndoCommand();

    move_command->setText(tr("Move Items"));

    while (!stream.atEnd()) {
      stream >> enabled_streams >> r >> item_ptr;

      Item* item = reinterpret_cast<Item*>(item_ptr);

      // Check if Item is already the drop location or if its parent is the drop location, in which case this is a
      // no-op

      if (item != drop_location && item->parent() != drop_location && !ItemIsParentOfChild(item, drop_location)) {
        MoveItemCommand* mic = new MoveItemCommand(this, item, static_cast<Folder*>(drop_location), move_command);

        Q_UNUSED(mic)
      }
    }

    Core::instance()->undo_stack()->pushIfHasChildren(move_command);

    return true;

  } else if (mime_formats.contains("text/uri-list")) {
    // We received a list of files
    QByteArray file_data = data->data("text/uri-list");

    // Use text stream to parse (just an easy way of sifting through line breaks
    QTextStream stream(&file_data);

    // Convert QByteArray to QStringList (which Core takes for importing)
    QStringList urls;
    while (!stream.atEnd()) {
      QUrl url = stream.readLine();

      if (!url.isEmpty()) {
        urls.append(url.toLocalFile());
      }
    }

    // Get folder dropped onto
    Item* drop_item = GetItemObjectFromIndex(drop);

    // If we didn't drop onto an item, find the nearest parent folder (should eventually terminate at root either way)
    while (!drop_item->CanHaveChildren()) {
      drop_item = drop_item->parent();
    }

    // Trigger an import
    Core::instance()->ImportFiles(urls, this, static_cast<Folder*>(drop_item));
  }

  return false;
}

void ProjectViewModel::AddChild(Item *parent, ItemPtr child)
{
  QModelIndex parent_index;

  if (parent != project_->root()) {
    parent_index = CreateIndexFromItem(parent);
  }

  beginInsertRows(parent_index, parent->child_count(), parent->child_count());

  parent->add_child(child);

  endInsertRows();
}

void ProjectViewModel::RemoveChild(Item *parent, Item *child)
{
  QModelIndex parent_index;

  if (parent != project_->root()) {
    parent_index = CreateIndexFromItem(parent);
  }

  int child_row = IndexOfChild(child);

  beginRemoveRows(parent_index, child_row, child_row);

  parent->remove_child(child);

  endRemoveRows();
}

void ProjectViewModel::RenameChild(Item *item, const QString &name)
{
  item->set_name(name);

  QModelIndex index = CreateIndexFromItem(item, columns_.indexOf(kName));

  emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
}

int ProjectViewModel::IndexOfChild(Item *item) const
{
  // Find parent's index within its own parent
  // (FIXME: this model should handle sorting, which means it'll have to "know" the indices)

  if (item == project_->root()) {
    return -1;
  }

  Item* parent = item->parent();

  if (parent != nullptr) {
    for (int i=0;i<parent->child_count();i++) {
      if (parent->child(i) == item) {
        return i;
      }
    }
  }

  return -1;
}

int ProjectViewModel::ChildCount(const QModelIndex &index)
{
  Item* item = GetItemObjectFromIndex(index);

  return item->child_count();
}

Item *ProjectViewModel::GetItemObjectFromIndex(const QModelIndex &index) const
{
  if (index.isValid()) {
    return static_cast<Item*>(index.internalPointer());
  }

  return project_->root();
}

bool ProjectViewModel::ItemIsParentOfChild(Item *parent, Item *child) const
{
  // Loop through parent hierarchy checking if `parent` is one of its parents
  do {
    child = child->parent();

    if (parent == child) {
      return true;
    }
  } while (child != nullptr);

  return false;
}

void ProjectViewModel::MoveItemInternal(Item *item, Item *destination)
{
  QModelIndex item_index = CreateIndexFromItem(item);

  QModelIndex destination_index = CreateIndexFromItem(destination);

  beginMoveRows(item_index.parent(), item_index.row(), item_index.row(), destination_index, destination->child_count());

  ItemPtr item_ptr = item->get_shared_ptr();

  destination->add_child(item_ptr);

  endMoveRows();
}

QModelIndex ProjectViewModel::CreateIndexFromItem(Item *item, int column)
{
  return createIndex(IndexOfChild(item), column, item);
}

ProjectViewModel::MoveItemCommand::MoveItemCommand(ProjectViewModel *model,
                                                   Item *item,
                                                   Folder *destination,
                                                   QUndoCommand *parent) :
  UndoCommand(parent),
  model_(model),
  item_(item),
  destination_(destination)
{
  source_ = static_cast<Folder*>(item->parent());

  setText(QCoreApplication::translate("MoveItemCommand", "Move Item"));
}

Project *ProjectViewModel::MoveItemCommand::GetRelevantProject() const
{
  return model_->project();
}

void ProjectViewModel::MoveItemCommand::redo_internal()
{
  model_->MoveItemInternal(item_, destination_);
}

void ProjectViewModel::MoveItemCommand::undo_internal()
{
  model_->MoveItemInternal(item_, source_);
}

ProjectViewModel::RenameItemCommand::RenameItemCommand(ProjectViewModel* model, Item *item, const QString &name, QUndoCommand *parent) :
  UndoCommand(parent),
  model_(model),
  item_(item),
  new_name_(name)
{
  old_name_ = item->name();

  setText(QCoreApplication::translate("RenameItemCommand", "Rename Item"));
}

Project *ProjectViewModel::RenameItemCommand::GetRelevantProject() const
{
  return model_->project();
}

void ProjectViewModel::RenameItemCommand::redo_internal()
{
  model_->RenameChild(item_, new_name_);
}

void ProjectViewModel::RenameItemCommand::undo_internal()
{
  model_->RenameChild(item_, old_name_);
}

ProjectViewModel::AddItemCommand::AddItemCommand(ProjectViewModel* model, Item* folder, ItemPtr child, QUndoCommand* parent) :
  UndoCommand(parent),
  model_(model),
  parent_(folder),
  child_(child),
  done_(false)
{
}

Project *ProjectViewModel::AddItemCommand::GetRelevantProject() const
{
  return model_->project();
}

void ProjectViewModel::AddItemCommand::redo_internal()
{
  model_->AddChild(parent_, child_);

  done_ = true;
}

void ProjectViewModel::AddItemCommand::undo_internal()
{
  model_->RemoveChild(parent_, child_.get());

  done_ = false;
}

ProjectViewModel::RemoveItemCommand::RemoveItemCommand(ProjectViewModel *model, ItemPtr item, QUndoCommand *parent) :
  UndoCommand(parent),
  model_(model),
  item_(item)
{
}

Project *ProjectViewModel::RemoveItemCommand::GetRelevantProject() const
{
  return model_->project();
}

void ProjectViewModel::RemoveItemCommand::redo_internal()
{
  parent_ = item_->parent();
  model_->RemoveChild(parent_, item_.get());
}

void ProjectViewModel::RemoveItemCommand::undo_internal()
{
  model_->AddChild(parent_, item_);
}

OLIVE_NAMESPACE_EXIT
