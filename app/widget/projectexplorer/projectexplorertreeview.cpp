#include "projectexplorertreeview.h"

#include <QMouseEvent>

ProjectExplorerTreeView::ProjectExplorerTreeView(QWidget *parent) :
  QTreeView(parent)
{
}

void ProjectExplorerTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
  // Perform default double click functions
  QTreeView::mouseDoubleClickEvent(event);

  // Get the index at whatever position was double clicked
  QModelIndex index = indexAt(event->pos());

  // Emit the signal with this index
  emit DoubleClickedView(index);
}
