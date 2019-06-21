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

private:
  Project* project_;

  QVector<ColumnType> columns_;
};

#endif // VIEWMODEL_H
