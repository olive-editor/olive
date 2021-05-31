/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "node/project/sequence/sequence.h"
#include "panel/footageviewer/footageviewer.h"
#include "panel/timeline/timeline.h"
#include "panel/panelmanager.h"
#include "widget/menu/menushared.h"
#include "widget/projecttoolbar/projecttoolbar.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

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
  connect(toolbar, &ProjectToolbar::SaveClicked, this, &ProjectPanel::SaveConnectedProject);

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

Folder *ProjectPanel::get_root() const
{
  return explorer_->get_root();
}

void ProjectPanel::set_root(Folder *item)
{
  explorer_->set_root(item);

  Retranslate();
}

QVector<Node *> ProjectPanel::SelectedItems() const
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

void ProjectPanel::Edit(Node* item)
{
  explorer_->Edit(item);
}

void ProjectPanel::Retranslate()
{
  if (project() && explorer_->get_root() != project()->root()) {
    SetTitle(tr("Folder"));
  } else {
    SetTitle(tr("Project"));
  }

  UpdateSubtitle();
}

void ProjectPanel::ItemDoubleClickSlot(Node *item)
{
  if (item == nullptr) {
    // If the user double clicks on empty space, show the import dialog
    Core::instance()->DialogImportShow();
  } else if (dynamic_cast<Footage*>(item)) {
    // Open this footage in a FootageViewer
    PanelManager::instance()->MostRecentlyFocused<FootageViewerPanel>()->ConnectViewerNode(static_cast<Footage*>(item));
  } else if (dynamic_cast<Sequence*>(item)) {
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

    if (explorer_->get_root() != project()->root()) {
      QString folder_path;

      Folder* item = explorer_->get_root();

      do {
        folder_path.prepend(QStringLiteral("/%1").arg(item->GetLabel()));

        item = item->folder();
      } while (item != project()->root());

      project_title.append(folder_path);
    }

    SetSubtitle(project_title);
  } else {
    SetSubtitle(tr("(none)"));
  }
}

void ProjectPanel::SaveConnectedProject()
{
  Core::instance()->SaveProject(this->project());
}

QVector<ViewerOutput *> ProjectPanel::GetSelectedFootage() const
{
  QVector<Node*> items = SelectedItems();
  QVector<ViewerOutput*> footage;

  foreach (Node* i, items) {
    if (dynamic_cast<ViewerOutput*>(i)) {
      footage.append(static_cast<ViewerOutput*>(i));
    }
  }

  return footage;
}

}
