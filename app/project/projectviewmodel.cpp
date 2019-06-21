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
  if (role == Qt::DisplayRole) {

    ColumnType column_type = columns_.at(index.column());

    switch (column_type) {
    case kName:
      return static_cast<Item*>(index.internalPointer())->name();
    case kDuration:
      return "00:00:00;00";
    case kRate:
      return "29.97 FPS";
    }
  }

  // TODO Add DecorationRole to column 1 for icons

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
