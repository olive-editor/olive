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

#include "projectexplorer.h"

#include <QDebug>

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QStackedWidget(parent),
  view_type_(olive::TreeView),
  model_(this)
{
  tree_view_ = new ProjectExplorerTreeView(this);
  AddView(tree_view_);

  list_view_ = new ProjectExplorerListView(this);
  AddView(list_view_);

  icon_view_ = new ProjectExplorerIconView(this);
  AddView(icon_view_);
}

const olive::ProjectViewType &ProjectExplorer::view_type()
{
  return view_type_;
}

void ProjectExplorer::set_view_type(olive::ProjectViewType type)
{
  view_type_ = type;

  // Set widget based on view type
  switch (view_type_) {
  case olive::TreeView:
    setCurrentWidget(tree_view_);
    break;
  case olive::ListView:
    setCurrentWidget(list_view_);
    break;
  case olive::IconView:
    setCurrentWidget(icon_view_);
    break;
  }
}

void ProjectExplorer::AddView(QAbstractItemView *view)
{
  view->setModel(&model_);
  connect(view, SIGNAL(DoubleClickedView(const QModelIndex&)), this, SLOT(DoubleClickViewSlot(const QModelIndex&)));
  addWidget(view);
}

void ProjectExplorer::DoubleClickViewSlot(const QModelIndex &index)
{
  if (index.isValid()) {

    // Retrieve source item from index
    Item* i = static_cast<Item*>(index.internalPointer());

    // Emit a signal
    emit DoubleClickedItem(i);

  } else {

    // Emit nullptr since no item was actually clicked on
    emit DoubleClickedItem(nullptr);

  }
}

Project *ProjectExplorer::project()
{
  return model_.project();
}

void ProjectExplorer::set_project(Project *p)
{
  model_.set_project(p);
}
