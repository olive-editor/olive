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

#include "projectmodel.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "project/media.h"
#include "debug.h"

ProjectModel olive::project_model;

ProjectModel::ProjectModel(QObject *parent) : QAbstractItemModel(parent), root_item(nullptr) {
	make_root();
}

ProjectModel::~ProjectModel() {
	destroy_root();
}

void ProjectModel::make_root() {
	root_item = new Media(nullptr);
	root_item->temp_id = 0;
	root_item->root = true;
}

void ProjectModel::destroy_root() {
	if (panel_sequence_viewer != nullptr) panel_sequence_viewer->viewer_widget->delete_function();
	if (panel_footage_viewer != nullptr) panel_footage_viewer->viewer_widget->delete_function();

	if (root_item != nullptr) {
		delete root_item;
	}
}

void ProjectModel::clear() {
	beginResetModel();
	destroy_root();
	make_root();
	endResetModel();
}

Media *ProjectModel::get_root() {
	return root_item;
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid())
		return QVariant();

	return static_cast<Media*>(index.internalPointer())->data(index.column(), role);
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const {
	if (!index.isValid())
		return Qt::ItemIsDropEnabled;

	return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

QVariant ProjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return root_item->data(section, role);

	return QVariant();
}

QModelIndex ProjectModel::index(int row, int column, const QModelIndex &parent) const {
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	Media *parentItem;

	if (!parent.isValid())
		parentItem = root_item;
	else
		parentItem = static_cast<Media*>(parent.internalPointer());

	Media *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex ProjectModel::create_index(int arow, int acolumn, void* adata) {
    return createIndex(arow, acolumn, adata);
}

QModelIndex ProjectModel::parent(const QModelIndex &index) const {
	if (!index.isValid())
		return QModelIndex();

	Media *childItem = static_cast<Media*>(index.internalPointer());
	Media *parentItem = childItem->parentItem();

	if (parentItem == root_item)
		return QModelIndex();

  return createIndex(parentItem->row(), 0, parentItem);
}

bool ProjectModel::canFetchMore(const QModelIndex &parent) const
{

  // Mostly implementing this because QSortFilterProxyModel will actually *ignore* a "true" result from hasChildren()
  // and then query this value for a "true" result. So we need to override both this and hasChildren() to show a
  // persistent childIndicator for folder items.

  if (parent.isValid()
      && static_cast<Media*>(parent.internalPointer())->get_type() == MEDIA_TYPE_FOLDER) {
    return true;
  }
  return QAbstractItemModel::canFetchMore(parent);
}

bool ProjectModel::hasChildren(const QModelIndex &parent) const
{
  if (parent.isValid()
      && static_cast<Media*>(parent.internalPointer())->get_type() == MEDIA_TYPE_FOLDER) {
    return true;
  }
  return QAbstractItemModel::hasChildren(parent);
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	if (role != Qt::EditRole)
		return false;

	Media *item = static_cast<Media*>(index.internalPointer());
	bool result = item->setData(index.column(), value);

	if (result)
		emit dataChanged(index, index);

	return result;
}

int ProjectModel::rowCount(const QModelIndex &parent) const {
  Media *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid()) {
		parentItem = root_item;
	} else {
		parentItem = static_cast<Media*>(parent.internalPointer());
	}

	return parentItem->childCount();
}

int ProjectModel::columnCount(const QModelIndex &parent) const {
	if (parent.isValid())
		return static_cast<Media*>(parent.internalPointer())->columnCount();
	else
		return root_item->columnCount();
}

Media *ProjectModel::getItem(const QModelIndex &index) const {
	if (index.isValid()) {
		Media *item = static_cast<Media*>(index.internalPointer());
		if (item)
			return item;
	}
	return root_item;
}

void ProjectModel::set_icon(Media* m, const QIcon &ico) {
	QModelIndex index = createIndex(m->row(), 0, m);
	m->set_icon(ico);
	emit dataChanged(index, index);
}

void ProjectModel::appendChild(Media *parent, Media *child) {
	if (parent == nullptr) parent = root_item;
	beginInsertRows(parent == root_item ? QModelIndex() : createIndex(parent->row(), 0, parent), parent->childCount(), parent->childCount());
	parent->appendChild(child);
	endInsertRows();
}

void ProjectModel::moveChild(Media *child, Media *to) {
	if (to == nullptr) to = root_item;
	Media* from = child->parentItem();
	beginMoveRows(
				from == root_item ? QModelIndex() : createIndex(from->row(), 0, from),
				child->row(),
				child->row(),
				to == root_item ? QModelIndex() : createIndex(to->row(), 0, to),
				to->childCount()
			);
	from->removeChild(child->row());
	to->appendChild(child);
	endMoveRows();
}

void ProjectModel::removeChild(Media* parent, Media* m) {
	if (parent == nullptr) parent = root_item;
	beginRemoveRows(parent == root_item ? QModelIndex() : createIndex(parent->row(), 0, parent), m->row(), m->row());
	parent->removeChild(m->row());
	endRemoveRows();
}

Media* ProjectModel::child(int i, Media* parent) {
	if (parent == nullptr) parent = root_item;
	return parent->child(i);
}

int ProjectModel::childCount(Media *parent) {
	if (parent == nullptr) parent = root_item;
	return parent->childCount();
}
