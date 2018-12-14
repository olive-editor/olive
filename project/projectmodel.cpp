#include "projectmodel.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "project/media.h"
#include "debug.h"

ProjectModel::ProjectModel(QObject *parent) : QAbstractItemModel(parent), root_item(NULL) {
    clear();
}

ProjectModel::~ProjectModel() {    
    destroy_root();
}

void ProjectModel::destroy_root() {
    if (panel_sequence_viewer != NULL) panel_sequence_viewer->viewer_widget->delete_function();
    if (panel_footage_viewer != NULL) panel_footage_viewer->viewer_widget->delete_function();

    if (root_item != NULL) {
        beginRemoveRows(QModelIndex(), 0, root_item->childCount());
        delete root_item;
        endRemoveRows();
    }
}

void ProjectModel::clear() {
    destroy_root();
    beginResetModel();
    root_item = new Media(0);
    root_item->root = true;
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

QModelIndex ProjectModel::parent(const QModelIndex &index) const {
    if (!index.isValid())
        return QModelIndex();

    Media *childItem = static_cast<Media*>(index.internalPointer());
    Media *parentItem = childItem->parentItem();

    if (parentItem == root_item)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
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

void ProjectModel::appendChild(Media *parent, Media *child) {
    if (parent == NULL) parent = root_item;
    beginInsertRows(parent == root_item ? QModelIndex() : createIndex(parent->row(), 0, parent), parent->childCount(), parent->childCount());
    parent->appendChild(child);
    endInsertRows();
}

void ProjectModel::moveChild(Media *child, Media *to) {
    if (to == NULL) to = root_item;
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
    if (parent == NULL) parent = root_item;
    beginRemoveRows(parent == root_item ? QModelIndex() : createIndex(parent->row(), 0, parent), m->row(), m->row());
    parent->removeChild(m->row());
    endRemoveRows();
}

Media* ProjectModel::child(int i, Media* parent) {
    if (parent == NULL) parent = root_item;
    return parent->child(i);
}

int ProjectModel::childCount(Media *parent) {
    if (parent == NULL) parent = root_item;
    return parent->childCount();
}
