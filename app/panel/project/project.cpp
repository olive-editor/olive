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

#include <QMenu>
#include <QVBoxLayout>

#include "core.h"
#include "panel/footageviewer/footageviewer.h"
#include "panel/timeline/timeline.h"
#include "panel/panelmanager.h"
#include "project/item/sequence/sequence.h"
#include "widget/menu/menushared.h"
#include "widget/projecttoolbar/projecttoolbar.h"
#include "window/mainwindow/mainwindow.h"

ProjectPanel::ProjectPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("ProjectPanel");

  // Create main widget and its layout
  QWidget* central_widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->setMargin(0);
  //layout->setSpacing(0);
  setWidget(central_widget);

  // Set up project toolbar
  ProjectToolbar* toolbar = new ProjectToolbar(this);
  layout->addWidget(toolbar);

  // Make toolbar connections
  connect(toolbar, SIGNAL(NewClicked()), this, SLOT(ShowNewMenu()));

  // Set up main explorer object
  explorer_ = new ProjectExplorer(this);
  layout->addWidget(explorer_);
  connect(explorer_, SIGNAL(DoubleClickedItem(Item*)), this, SLOT(ItemDoubleClickSlot(Item*)));

  // Set toolbar's view to the explorer's view
  toolbar->SetView(explorer_->view_type());

  // Connect toolbar's view change signal to the explorer's view change slot
  connect(toolbar,
          &ProjectToolbar::ViewChanged,
          explorer_,
          &ProjectExplorer::set_view_type);

  // Set strings
  Retranslate();
}

Project *ProjectPanel::project()
{
  return explorer_->project();
}

void ProjectPanel::set_project(Project *p)
{
  if (project()) {
    disconnect(project(), &Project::NameChanged, this, &ProjectPanel::ProjectNameChanged);
  }

  explorer_->set_project(p);

  if (project()) {
    connect(project(), &Project::NameChanged, this, &ProjectPanel::ProjectNameChanged);
  }

  ProjectNameChanged();
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

void ProjectPanel::SelectAll()
{
  explorer_->SelectAll();
}

void ProjectPanel::DeselectAll()
{
  explorer_->DeselectAll();
}

void ProjectPanel::Insert()
{
  TimelinePanel* timeline = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();

  if (timeline) {
    timeline->InsertFootageAtPlayhead(GetSelectedFootage());
  }
}

void ProjectPanel::Overwrite()
{
  TimelinePanel* timeline = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();

  if (timeline) {
    timeline->OverwriteFootageAtPlayhead(GetSelectedFootage());
  }
}

void ProjectPanel::DeleteSelected()
{
  explorer_->DeleteSelected();
}

void ProjectPanel::Edit(Item* item)
{
  explorer_->Edit(item);
}

void ProjectPanel::Retranslate()
{
  SetTitle(tr("Project"));

  ProjectNameChanged();
}

void ProjectPanel::ItemDoubleClickSlot(Item *item)
{
  if (item == nullptr) {
    // If the user double clicks on empty space, show the import dialog
    Core::instance()->DialogImportShow();
  } else if (item->type() == Item::kFootage) {
    // Open this footage in a FootageViewer
    PanelManager::instance()->MostRecentlyFocused<FootageViewerPanel>()->SetFootage(static_cast<Footage*>(item));
  } else if (item->type() == Item::kSequence) {
    // Open this sequence in the Timeline
    Core::instance()->main_window()->OpenSequence(static_cast<Sequence*>(item));
  }
}

void ProjectPanel::ShowNewMenu()
{
  Menu new_menu(this);

  MenuShared::instance()->AddItemsForNewMenu(&new_menu);

  new_menu.exec(QCursor::pos());
}

void ProjectPanel::ProjectNameChanged()
{
  if (project() == nullptr) {
    SetSubtitle(tr("(none)"));
  } else {
    SetSubtitle(project()->name());
  }
}

QList<Footage *> ProjectPanel::GetSelectedFootage()
{
  QList<Item*> items = SelectedItems();
  QList<Footage*> footage;

  foreach (Item* i, items) {
    if (i->type() == Item::kFootage) {
      footage.append(static_cast<Footage*>(i));
    }
  }

  return footage;
}
