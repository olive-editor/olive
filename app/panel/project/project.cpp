/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

OLIVE_NAMESPACE_ENTER

ProjectPanel::ProjectPanel(QWidget *parent) :
  PanelWidget(QStringLiteral("ProjectPanel"), parent)
{
  // Create main widget and its layout
  QWidget* central_widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->setMargin(0);

  SetWidgetWithPadding(central_widget);

  // Set up project toolbar
  ProjectToolbar* toolbar = new ProjectToolbar(this);
  layout->addWidget(toolbar);

  // Make toolbar connections
  connect(toolbar, &ProjectToolbar::NewClicked, this, &ProjectPanel::ShowNewMenu);
  connect(toolbar, &ProjectToolbar::OpenClicked, Core::instance(), &Core::OpenProject);
  connect(toolbar, &ProjectToolbar::SaveClicked, Core::instance(), &Core::SaveActiveProject);
  connect(toolbar, &ProjectToolbar::UndoClicked, Core::instance()->undo_stack(), &QUndoStack::undo);
  connect(toolbar, &ProjectToolbar::RedoClicked, Core::instance()->undo_stack(), &QUndoStack::redo);

  // Set up main explorer object
  explorer_ = new ProjectExplorer(this);
  layout->addWidget(explorer_);
  connect(explorer_, &ProjectExplorer::DoubleClickedItem, this, &ProjectPanel::ItemDoubleClickSlot);

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

Project* ProjectPanel::project() const
{
  return explorer_->project();
}

void ProjectPanel::set_project(Project* p)
{
  if (project()) {
    disconnect(project(), &Project::NameChanged, this, &ProjectPanel::UpdateSubtitle);
    disconnect(project(), &Project::NameChanged, this, &ProjectPanel::ProjectNameChanged);
    disconnect(project(), &Project::ModifiedChanged, this, &ProjectPanel::setWindowModified);
  }

  explorer_->set_project(p);

  if (project()) {
    connect(project(), &Project::NameChanged, this, &ProjectPanel::UpdateSubtitle);
    connect(project(), &Project::NameChanged, this, &ProjectPanel::ProjectNameChanged);
    connect(project(), &Project::ModifiedChanged, this, &ProjectPanel::setWindowModified);
  }

  UpdateSubtitle();

  emit ProjectNameChanged();

  if (p) {
    setWindowModified(p->is_modified());
  } else {
    setWindowModified(false);
  }
}

QModelIndex ProjectPanel::get_root_index() const
{
  return explorer_->get_root_index();
}

void ProjectPanel::set_root(Item *item)
{
  explorer_->set_root(item);

  Retranslate();
}

QList<Item *> ProjectPanel::SelectedItems() const
{
  return explorer_->SelectedItems();
}

Folder *ProjectPanel::GetSelectedFolder() const
{
  return explorer_->GetSelectedFolder();
}

ProjectViewModel *ProjectPanel::model() const
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
  if (explorer_->get_root_index().isValid()) {
    SetTitle(tr("Folder"));
  } else {
    SetTitle(tr("Project"));
  }

  UpdateSubtitle();
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

void ProjectPanel::UpdateSubtitle()
{
  if (project()) {
    QString project_title = QStringLiteral("[*]%1").arg(project()->name());

    if (explorer_->get_root_index().isValid()) {
      QString folder_path;

      Item* item = static_cast<Item*>(explorer_->get_root_index().internalPointer());

      do {
        folder_path.prepend(QStringLiteral("/%1").arg(item->name()));

        item = item->parent();
      } while (item != project()->root());

      project_title.append(folder_path);
    }

    SetSubtitle(project_title);
  } else {
    SetSubtitle(tr("(none)"));
  }
}

QList<Footage *> ProjectPanel::GetSelectedFootage() const
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

OLIVE_NAMESPACE_EXIT
