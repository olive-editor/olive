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

#include "project.h"

#include <QVBoxLayout>

#include "core.h"
#include "widget/projecttoolbar/projecttoolbar.h"

ProjectPanel::ProjectPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // Create main widget and its layout
  QWidget* central_widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->setMargin(0);
  //layout->setSpacing(0);
  setWidget(central_widget);

  // Set up project toolbar
  ProjectToolbar* toolbar = new ProjectToolbar(this);
  layout->addWidget(toolbar);

  // Set up main explorer object
  explorer_ = new ProjectExplorer(this);
  layout->addWidget(explorer_);
  connect(explorer_, SIGNAL(DoubleClickedItem(Item*)), this, SLOT(ItemDoubleClickSlot(Item*)));

  // Set toolbar's view to the explorer's view
  toolbar->SetView(explorer_->view_type());

  // Connect toolbar's view change signal to the explorer's view change slot
  connect(toolbar,
          SIGNAL(ViewChanged(olive::ProjectViewType)),
          explorer_,
          SLOT(set_view_type(olive::ProjectViewType)));

  // Set strings
  Retranslate();
}

Project *ProjectPanel::project()
{
  return explorer_->project();
}

void ProjectPanel::set_project(Project *p)
{
  explorer_->set_project(p);

  Retranslate();
}

QList<Item *> ProjectPanel::SelectedItems()
{
  return explorer_->SelectedItems();
}

Folder *ProjectPanel::GetSelectedFolder()
{
  return explorer_->GetSelectedFolder();
}

ProjectViewModel *ProjectPanel::model()
{
  return explorer_->model();
}

void ProjectPanel::Edit(Item* item)
{
  explorer_->Edit(item);
}

void ProjectPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void ProjectPanel::Retranslate()
{
  SetTitle(tr("Project"));

  if (project() == nullptr) {
    SetSubtitle(tr("(none)"));
  } else {
    SetSubtitle(project()->name());
  }
}

void ProjectPanel::ItemDoubleClickSlot(Item *item)
{
  if (item == nullptr) {
    // If the user double clicks on empty space, show the import dialog
    olive::core.DialogImportShow();
  }

  // FIXME: Double click Item should do something
}
