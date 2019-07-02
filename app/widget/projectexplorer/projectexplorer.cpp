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
#include <QVBoxLayout>

#include "projectexplorerdefines.h"

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QWidget(parent),
  view_type_(olive::TreeView),
  model_(this)
{
  // Create layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  // Set up navigation bar
  nav_bar_ = new ProjectExplorerNavigation(this);
  connect(nav_bar_, SIGNAL(SizeChanged(int)), this, SLOT(SizeChangedSlot(int)));
  layout->addWidget(nav_bar_);

  // Set up stacked widget
  stacked_widget_ = new QStackedWidget(this);
  layout->addWidget(stacked_widget_);

  // Add tree view to stacked widget
  tree_view_ = new ProjectExplorerTreeView(stacked_widget_);
  AddView(tree_view_);

  // Add list view to stacked widget
  list_view_ = new ProjectExplorerListView(stacked_widget_);
  AddView(list_view_);

  // Add icon view to stacked widget
  icon_view_ = new ProjectExplorerIconView(stacked_widget_);
  AddView(icon_view_);

  // Set default view to tree view
  set_view_type(olive::TreeView);

  // Set default icon size
  SizeChangedSlot(olive::kProjectIconSizeDefault);
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
    stacked_widget_->setCurrentWidget(tree_view_);
    nav_bar_->setVisible(false);
    break;
  case olive::ListView:
    stacked_widget_->setCurrentWidget(list_view_);
    nav_bar_->setVisible(true);
    break;
  case olive::IconView:
    stacked_widget_->setCurrentWidget(icon_view_);
    nav_bar_->setVisible(true);
    break;
  }
}

void ProjectExplorer::AddView(QAbstractItemView *view)
{
  view->setModel(&model_);
  connect(view, SIGNAL(DoubleClickedView(const QModelIndex&)), this, SLOT(DoubleClickViewSlot(const QModelIndex&)));
  stacked_widget_->addWidget(view);
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

void ProjectExplorer::SizeChangedSlot(int s)
{
  icon_view_->setGridSize(QSize(s, s));

  list_view_->setIconSize(QSize(s, s));
}

Project *ProjectExplorer::project()
{
  return model_.project();
}

void ProjectExplorer::set_project(Project *p)
{
  model_.set_project(p);
}
