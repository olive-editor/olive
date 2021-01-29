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

#ifndef VIEWMODEL_H
#define VIEWMODEL_H

#include <QAbstractItemModel>

#include "project.h"
#include "undo/undocommand.h"
#include "node/block/block.h"

namespace olive {

/**
 * @brief An adapter that interprets the data in a Project into a Qt item model for usage in ViewModel Views.
 *
 * Assuming a Project is currently "open" (i.e. the Project is connected to a ProjectExplorer/ProjectPanel through
 * a ProjectViewModel), it may be better to make modifications (e.g. additions/removals/renames) through the
 * ProjectViewModel so that the views can be efficiently and correctly updated. ProjectViewModel contains several
 * "wrapper" functions for Project and Item functions that also signal any connected views to update accordingly.
 */
class ProjectViewModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum ColumnType {
    /// Media name
    kName,

    /// Media duration
    kDuration,

    /// Media rate (frame rate for video, sample rate for audio)
    kRate
  };

  /**
   * @brief ProjectViewModel Constructor
   *
   * @param parent
   * Parent object for memory handling
   */
  ProjectViewModel(QObject* parent);

  /**
   * @brief Get currently active project
   *
   * @return
   *
   * Currently active project or nullptr if there is none
   */
  Project* project() const;

  /**
   * @brief Set the project to adapt
   *
   * Any views attached to this model will get updated by this function.
   *
   * @param p
   *
   * Project to adapt, can be set to nullptr to "close" the project (will show an empty model that cannot be modified)
   */
  void set_project(Project* p);

  /** Compulsory Qt QAbstractItemModel overrides */
  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &child) const override;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  /** Optional Qt QAbstractItemModel overrides */
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  virtual bool canFetchMore(const QModelIndex &parent) const override;

  /** Drag and drop support */
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData * mimeData(const QModelIndexList &indexes) const override;
  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

  /** Other model functions */
  void AddChild(Item* parent, Item* child);
  void RemoveChild(Item* parent, Item* child, QObject* new_parent);
  void RenameChild(Item* item, const QString& name);

  /**
   * @brief Convenience function for creating QModelIndexes from an Item object
   */
  QModelIndex CreateIndexFromItem(Item* item, int column = 0);

  /**
   * @brief An UndoCommand for moving an item from one folder to another folder
   */
  class MoveItemCommand : public UndoCommand {
  public:
    MoveItemCommand(ProjectViewModel* model, Item* item, Folder* destination);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;

    virtual void undo() override;

  private:
    ProjectViewModel* model_;
    Item* item_;
    Folder* source_;
    Folder* destination_;

  };

  /**
   * @brief An UndoCommand for renaming an item
   */
  class RenameItemCommand : public UndoCommand {
  public:
    RenameItemCommand(ProjectViewModel* model, Item* item, const QString& name);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;

    virtual void undo() override;

  private:
    ProjectViewModel* model_;
    Item* item_;
    QString old_name_;
    QString new_name_;
  };

  /**
   * @brief An UndoCommand for adding an item
   */
  class AddItemCommand : public UndoCommand {
  public:
    AddItemCommand(ProjectViewModel* model, Item* folder, Item *child);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;

    virtual void undo() override;

  private:
    ProjectViewModel* model_;
    Item* parent_;
    Item* child_;
    QObject memory_manager_;

  };

  /**
   * @brief An undo command for removing an item
   */
  class RemoveItemCommand : public UndoCommand {
  public:
    RemoveItemCommand(ProjectViewModel* model, Item* item);

    virtual Project* GetRelevantProject() const override;

    virtual void redo() override;

    virtual void undo() override;

  private:
    ProjectViewModel* model_;
    Item* item_;
    Item* parent_;
    QObject memory_manager_;

  };

private:
  /**
   * @brief Retrieve the index of `item` in its parent
   *
   * This function will return the index of a specified item in its parent according to whichever sorting algorithm
   * is currently active.
   *
   * @return
   *
   * Index of the specified item, or -1 if the item is root (in which case it has no parent).
   */
  int IndexOfChild(Item* item) const;

  /**
   * @brief Get the child count of an index
   *
   * @param index
   *
   * @return
   *
   * Return number of children (immediate children only)
   */
  int ChildCount(const QModelIndex& index);

  /**
   * @brief Retrieves the Item object from a given index
   *
   * A convenience function for retrieving Item objects. If the index is not valid, this returns the root Item.
   */
  Item* GetItemObjectFromIndex(const QModelIndex& index) const;

  /**
   * @brief Check if an Item is a parent of a Child
   *
   * Checks entire "parent hierarchy" of `child` to see if `parent` is one of its parents.
   */
  bool ItemIsParentOfChild(Item* parent, Item* child) const;

  /**
   * @brief Moves an item to a new destination updating all views in the process
   *
   * This function will emit a signal indicating that rows are moving, set `destination` as the new parent of `item`,
   * and then emit a signal that the row has finished moving.
   *
   * It's not recommended to use this function directly in most cases since it does not create an UndoCommand allowing
   * the user to undo the move. Instead this function should primarily be called from UndoCommands belonging to this
   * class (e.g. MoveItemCommand).
   */
  void MoveItemInternal(Item* item, Item* destination);

  Project* project_;

  QVector<ColumnType> columns_;
};

}

#endif // VIEWMODEL_H
