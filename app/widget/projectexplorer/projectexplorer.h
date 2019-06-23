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
class ProjectExplorer : public QStackedWidget
{
public:
  enum ViewType {
    TreeView,
    ListView,
    IconView
  };

  ProjectExplorer(QWidget* parent);

  const ViewType& view_type();
  void set_view_type(const ViewType& type);

  Project* project();
  void set_project(Project* p);

private:
  QTreeView* tree_view_;

  ViewType view_type_;

  ProjectViewModel model_;
};

#endif // PROJECTEXPLORER_H
