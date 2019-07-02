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

#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include <QStackedWidget>
#include <QTreeView>

#include "project/project.h"
#include "project/projectviewmodel.h"
#include "project/projectviewtype.h"
#include "widget/projectexplorer/projectexplorericonview.h"
#include "widget/projectexplorer/projectexplorerlistview.h"
#include "widget/projectexplorer/projectexplorertreeview.h"
#include "widget/projectexplorer/projectexplorernavigation.h"

/**
 * @brief The ProjectExplorer class
 *
 * A widget for browsing through a Project structure.
 *
 * ProjectExplorer automatically handles the view<->model system using a ProjectViewModel. Therefore, all that needs to
 * be provided is the Project structure itself.
 *
 * This widget contains three views, tree view, list view, and icon view. These can be switched at any time.
 */
class ProjectExplorer : public QWidget
{
  Q_OBJECT
public:
  ProjectExplorer(QWidget* parent);

  const olive::ProjectViewType& view_type();

  Project* project();
  void set_project(Project* p);

public slots:
  void set_view_type(olive::ProjectViewType type);

signals:
  /**
   * @brief Emitted when an Item is double clicked
   *
   * @param item
   *
   * The Item that was double clicked, or nullptr if empty area was double clicked
   */
  void DoubleClickedItem(Item* item);

private:
  /**
   * @brief Simple convenience function for adding a view to this stacked widget
   *
   * Mainly for use in the constructor. Adds the view, connects its signals/slots, and sets the model.
   *
   * @param view
   *
   * View to add to the stack
   */
  void AddView(QAbstractItemView* view);

  QStackedWidget* stacked_widget_;

  ProjectExplorerNavigation* nav_bar_;

  ProjectExplorerIconView* icon_view_;
  ProjectExplorerListView* list_view_;
  ProjectExplorerTreeView* tree_view_;

  olive::ProjectViewType view_type_;

  ProjectViewModel model_;

private slots:
  void DoubleClickViewSlot(const QModelIndex& index);

  void SizeChangedSlot(int s);
};

#endif // PROJECTEXPLORER_H
