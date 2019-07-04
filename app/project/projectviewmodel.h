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

#ifndef VIEWMODEL_H
#define VIEWMODEL_H

#include <QAbstractItemModel>

#include "project.h"

/**
 * @brief The ProjectViewModel class
 *
 * An adapter that interprets the data in a Project into a Qt item model for usage in ViewModel Views.
 */
class ProjectViewModel : public QAbstractItemModel
{
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
  Project* project();

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

  /** Drag and drop support */
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData * mimeData(const QModelIndexList &indexes) const override;
  virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
private:
  int indexOfChild(Item* item) const;

  Project* project_;

  QVector<ColumnType> columns_;
};

#endif // VIEWMODEL_H
