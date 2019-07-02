#ifndef PROJECTEXPLORERTREEVIEW_H
#define PROJECTEXPLORERTREEVIEW_H

#include <QTreeView>

/**
 * @brief The ProjectExplorerTreeView class
 *
 * A fairly simple subclass of QTreeView that provides a double clicked signal whether the index is valid or not
 * (QAbstractItemView has a doubleClicked() signal but it's only emitted with a valid index).
 */
class ProjectExplorerTreeView : public QTreeView
{
  Q_OBJECT
public:
  ProjectExplorerTreeView(QWidget* parent);

protected:
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

signals:
  void DoubleClickedView(const QModelIndex& index);
};

#endif // PROJECTEXPLORERTREEVIEW_H
