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

#include "projectexplorer.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include <QVBoxLayout>

#include "common/define.h"
#include "core.h"
#include "dialog/sequence/sequence.h"
#include "projectexplorerundo.h"
#include "task/precache/precachetask.h"
#include "task/taskmanager.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "window/mainwindow/mainwindow.h"
#include "widget/timelinewidget/timelinewidget.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QWidget(parent),
  model_(this)
{
  // Create layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  // Set up navigation bar
  nav_bar_ = new ProjectExplorerNavigation(this);
  connect(nav_bar_, &ProjectExplorerNavigation::SizeChanged, this, &ProjectExplorer::SizeChangedSlot);
  connect(nav_bar_, &ProjectExplorerNavigation::DirectoryUpClicked, this, &ProjectExplorer::DirUpSlot);
  layout->addWidget(nav_bar_);

  // Set up stacked widget
  stacked_widget_ = new QStackedWidget(this);
  layout->addWidget(stacked_widget_);

  // Set up sort filter proxy model
  sort_model_.setSourceModel(&model_);

  // Add tree view to stacked widget
  tree_view_ = new ProjectExplorerTreeView(stacked_widget_);
  tree_view_->setSortingEnabled(true);
  tree_view_->sortByColumn(0, Qt::AscendingOrder);
  tree_view_->setContextMenuPolicy(Qt::CustomContextMenu);
  AddView(tree_view_);

  // Add list view to stacked widget
  list_view_ = new ProjectExplorerListView(stacked_widget_);
  list_view_->setContextMenuPolicy(Qt::CustomContextMenu);
  AddView(list_view_);

  // Add icon view to stacked widget
  icon_view_ = new ProjectExplorerIconView(stacked_widget_);
  icon_view_->setContextMenuPolicy(Qt::CustomContextMenu);
  AddView(icon_view_);

  // Set default view to tree view
  set_view_type(ProjectToolbar::TreeView);

  // Set default icon size
  SizeChangedSlot(kProjectIconSizeDefault);

  // Set rename timer timeout
  rename_timer_.setInterval(500);
  connect(&rename_timer_, &QTimer::timeout, this, &ProjectExplorer::RenameTimerSlot);

  connect(tree_view_, &ProjectExplorerTreeView::customContextMenuRequested, this, &ProjectExplorer::ShowContextMenu);
  connect(list_view_, &ProjectExplorerListView::customContextMenuRequested, this, &ProjectExplorer::ShowContextMenu);
  connect(icon_view_, &ProjectExplorerIconView::customContextMenuRequested, this, &ProjectExplorer::ShowContextMenu);
}

const ProjectToolbar::ViewType &ProjectExplorer::view_type() const
{
  return view_type_;
}

void ProjectExplorer::set_view_type(ProjectToolbar::ViewType type)
{
  view_type_ = type;

  // Set widget based on view type
  switch (view_type_) {
  case ProjectToolbar::TreeView:
    stacked_widget_->setCurrentWidget(tree_view_);
    nav_bar_->setVisible(false);
    break;
  case ProjectToolbar::ListView:
    stacked_widget_->setCurrentWidget(list_view_);
    nav_bar_->setVisible(true);
    break;
  case ProjectToolbar::IconView:
    stacked_widget_->setCurrentWidget(icon_view_);
    nav_bar_->setVisible(true);
    break;
  }
}

void ProjectExplorer::Edit(Node *item)
{
  CurrentView()->edit(sort_model_.mapFromSource(model_.CreateIndexFromItem(item)));
}

void ProjectExplorer::AddView(QAbstractItemView *view)
{
  view->setModel(&sort_model_);
  view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  connect(view, &QAbstractItemView::clicked, this, &ProjectExplorer::ItemClickedSlot);
  connect(view, &QAbstractItemView::doubleClicked, this, &ProjectExplorer::ItemDoubleClickedSlot);
  connect(view, SIGNAL(DoubleClickedEmptyArea()), this, SLOT(ViewEmptyAreaDoubleClickedSlot()));
  stacked_widget_->addWidget(view);
}

void ProjectExplorer::BrowseToFolder(const QModelIndex &index)
{
  // Make sure any rename timers are stopped
  rename_timer_.stop();

  // Set appropriate views to this index
  icon_view_->setRootIndex(index);
  list_view_->setRootIndex(index);

  // Set navbar text to folder's name
  if (index.isValid()) {
    Folder* f = static_cast<Folder*>(sort_model_.mapToSource(index).internalPointer());
    nav_bar_->set_text(f->GetLabel());
  } else {
    // Or set it to an empty string if the index is valid (which means we're browsing to the root directory)
    nav_bar_->set_text(QString());
  }

  // Set directory up enabled button based on whether we're in root or not
  nav_bar_->set_dir_up_enabled(index.isValid());
}

QAbstractItemView *ProjectExplorer::CurrentView() const
{
  return static_cast<QAbstractItemView*>(stacked_widget_->currentWidget());
}

void ProjectExplorer::ItemClickedSlot(const QModelIndex &index)
{
  if (index.isValid()) {
    if (clicked_index_ == index) {
      // The item has been clicked more than once, start a timer for renaming
      rename_timer_.start();
    } else {
      // Cache this index for the next click
      clicked_index_ = index;

      // If the rename timer had started, stop it now
      rename_timer_.stop();
    }
  } else {
    // Stop the rename timer
    rename_timer_.stop();
  }
}

void ProjectExplorer::ViewEmptyAreaDoubleClickedSlot()
{
  // Ensure no attempts to rename are made
  clicked_index_ = QModelIndex();
  rename_timer_.stop();

  emit DoubleClickedItem(nullptr);
}

void ProjectExplorer::ItemDoubleClickedSlot(const QModelIndex &index)
{
  // Ensure no attempts to rename are made
  clicked_index_ = QModelIndex();
  rename_timer_.stop();

  // Retrieve source item from index
  Node* i = static_cast<Node*>(sort_model_.mapToSource(index).internalPointer());

  // If the item is a folder, browse to it
  if (dynamic_cast<Folder*>(i) && (view_type() == ProjectToolbar::ListView || view_type() == ProjectToolbar::IconView)) {

    BrowseToFolder(index);

  }

  // Emit a signal
  emit DoubleClickedItem(i);
}

void ProjectExplorer::SizeChangedSlot(int s)
{
  icon_view_->setGridSize(QSize(s, s));

  list_view_->setIconSize(QSize(s, s));
}

void ProjectExplorer::DirUpSlot()
{
  QModelIndex current_root = icon_view_->rootIndex();

  if (current_root.isValid()) {
    QModelIndex parent = current_root.parent();

    BrowseToFolder(parent);
  }
}

void ProjectExplorer::RenameTimerSlot()
{
  // Start editing this index
  CurrentView()->edit(clicked_index_);

  // Reset clicked index state
  clicked_index_ = QModelIndex();

  // Stop rename timer
  rename_timer_.stop();
}

void ProjectExplorer::ShowContextMenu()
{
  Menu menu;
  Menu new_menu;

  context_menu_items_ = SelectedItems();

  if (context_menu_items_.isEmpty()) {
    // Items to show if no items are selected

    // "New" menu
    new_menu.setTitle(tr("&New"));
    MenuShared::instance()->AddItemsForNewMenu(&new_menu);
    menu.addMenu(&new_menu);

    // "Import" action
    QAction* import_action = menu.addAction(tr("&Import..."));
    connect(import_action, &QAction::triggered, Core::instance(), &Core::DialogImportShow);
  } else {

    // Actions to add when only one item is selected
    if (context_menu_items_.size() == 1) {
      Node* context_menu_item = context_menu_items_.first();

      if (dynamic_cast<Folder*>(context_menu_item)) {

        QAction* open_in_new_tab = menu.addAction(tr("Open in New Tab"));
        connect(open_in_new_tab, &QAction::triggered, this, &ProjectExplorer::OpenContextMenuItemInNewTab);

        QAction* open_in_new_window = menu.addAction(tr("Open in New Window"));
        connect(open_in_new_window, &QAction::triggered, this, &ProjectExplorer::OpenContextMenuItemInNewWindow);

      } else if (dynamic_cast<Footage*>(context_menu_item)) {

        QString reveal_text;

#if defined(Q_OS_WINDOWS)
        reveal_text = tr("Reveal in Explorer");
#elif defined(Q_OS_MAC)
        reveal_text = tr("Reveal in Finder");
#else
        reveal_text = tr("Reveal in File Manager");
#endif

        QAction* reveal_action = menu.addAction(reveal_text);
        connect(reveal_action, &QAction::triggered, this, &ProjectExplorer::RevealSelectedFootage);

      }

      menu.addSeparator();
    }

    bool all_items_are_footage = true;
    bool all_items_have_video_streams = true;
    bool all_items_are_footage_or_sequence = true;

    foreach (Node* i, context_menu_items_) {
      Footage* footage_cast_test = dynamic_cast<Footage*>(i);
      Sequence* sequence_cast_test = dynamic_cast<Sequence*>(i);

      if (footage_cast_test && !footage_cast_test->HasEnabledVideoStreams()) {
        all_items_have_video_streams = false;
      }

      if (!footage_cast_test) {
        all_items_are_footage = false;
      }

      if (!footage_cast_test && !sequence_cast_test) {
        all_items_are_footage_or_sequence = false;
      }
    }

    if (all_items_are_footage && all_items_have_video_streams) {
      Menu* proxy_menu = new Menu(tr("Pre-Cache"), &menu);
      menu.addMenu(proxy_menu);

      QVector<Sequence*> sequences = project()->root()->ListChildrenOfType<Sequence>();

      if (sequences.isEmpty()) {
        QAction* a = proxy_menu->addAction(tr("No sequences exist in project"));
        a->setEnabled(false);
      } else {
        foreach (Sequence* i, sequences) {
          QAction* a = proxy_menu->addAction(tr("For \"%1\"").arg(i->GetLabel()));
          a->setData(Node::PtrToValue(i));
        }

        connect(proxy_menu, &Menu::triggered, this, &ProjectExplorer::ContextMenuStartProxy);
      }
    }

    Q_UNUSED(all_items_are_footage_or_sequence)

    if (context_menu_items_.size() == 1) {
      menu.addSeparator();

      QAction* properties_action = menu.addAction(tr("P&roperties"));
      connect(properties_action, &QAction::triggered, this, &ProjectExplorer::ShowItemPropertiesDialog);
    }
  }

  menu.exec(QCursor::pos());
}

void ProjectExplorer::ShowItemPropertiesDialog()
{
  Node* sel = context_menu_items_.first();

  // FIXME: Support for multiple items
  if (dynamic_cast<Footage*>(sel)) {

    Core::instance()->LabelNodes({static_cast<Footage*>(sel)});

  } else if (dynamic_cast<Folder*>(sel)) {

    Core::instance()->LabelNodes({static_cast<Folder*>(sel)});

  } else if (dynamic_cast<Sequence*>(sel)) {

    SequenceDialog sd(static_cast<Sequence*>(sel), SequenceDialog::kExisting, this);
    sd.exec();

  }
}

void ProjectExplorer::RevealSelectedFootage()
{
  Footage* footage = static_cast<Footage*>(context_menu_items_.first());

#if defined(Q_OS_WINDOWS)
  // Explorer
  QStringList args;
  args << "/select," << QDir::toNativeSeparators(footage->filename());
  QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MAC)
  QStringList args;
  args << "-e";
  args << "tell application \"Finder\"";
  args << "-e";
  args << "activate";
  args << "-e";
  args << "select POSIX file \""+footage->filename()+"\"";
  args << "-e";
  args << "end tell";
  QProcess::startDetached("osascript", args);
#else
  QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(footage->filename()).dir().absolutePath()));
#endif
}

void ProjectExplorer::OpenContextMenuItemInNewTab()
{
  Core::instance()->main_window()->FolderOpen(project(), static_cast<Folder*>(context_menu_items_.first()), false);
}

void ProjectExplorer::OpenContextMenuItemInNewWindow()
{
  Core::instance()->main_window()->FolderOpen(project(), static_cast<Folder*>(context_menu_items_.first()), true);
}

void ProjectExplorer::ContextMenuStartProxy(QAction *a)
{
  Sequence* sequence = Node::ValueToPtr<Sequence>(a->data());

  // To get here, the `context_menu_items_` must be all kFootage
  foreach (Node* i, context_menu_items_) {
    Footage* f = static_cast<Footage*>(i);

    QVector<VideoParams> enabled_streams = f->GetEnabledVideoStreams();

    foreach (const VideoParams& stream, enabled_streams) {
      // Start a background task for proxying
      PreCacheTask* proxy_task = new PreCacheTask(f, stream.stream_index(), sequence);
      TaskManager::instance()->AddTask(proxy_task);
    }
  }
}

Project *ProjectExplorer::project() const
{
  return model_.project();
}

void ProjectExplorer::set_project(Project *p)
{
  model_.set_project(p);
}

QModelIndex ProjectExplorer::get_root_index() const
{
  return tree_view_->rootIndex();
}

void ProjectExplorer::set_root(Folder *item)
{
  QModelIndex index = sort_model_.mapFromSource(model_.CreateIndexFromItem(item));

  BrowseToFolder(index);
  tree_view_->setRootIndex(index);
}

QVector<Node *> ProjectExplorer::SelectedItems() const
{
  // Determine which view is active and get its selected indexes
  QModelIndexList index_list = CurrentView()->selectionModel()->selectedRows();

  // Convert indexes to item objects
  QVector<Node*> selected_items;

  for (int i=0;i<index_list.size();i++) {
    QModelIndex index = sort_model_.mapToSource(index_list.at(i));

    Node* item = static_cast<Node*>(index.internalPointer());

    selected_items.append(item);
  }

  return selected_items;
}

Folder *ProjectExplorer::GetSelectedFolder() const
{
  if (project() == nullptr) {
    return nullptr;
  }

  Folder* folder = nullptr;

  // Get the selected items from the panel
  QVector<Node*> selected_items = SelectedItems();

  // Heuristic for finding the selected folder:
  //
  // - If `folder` is nullptr, we set the first folder we find. Either the item itself if it's a folder, or the
  //   item's parent.
  // - Otherwise, if all folders found are the same, we'll use that to import into.
  // - If more than one folder is found, we play it safe and import into the root folder

  for (int i=0;i<selected_items.size();i++) {
    Node* sel_item = selected_items.at(i);

    // If this item is not a folder, presumably it's parent is
    if (!dynamic_cast<Folder*>(sel_item)) {
      sel_item = sel_item->folder();
    }

    if (folder == nullptr) {
      // If the folder is nullptr, cache it as this folder
      folder = static_cast<Folder*>(sel_item);
    } else if (folder != sel_item) {
      // If not, we've already cached a folder so we check if it's the same
      // If it isn't, we "play it safe" and use the root folder
      folder = nullptr;
      break;
    }
  }

  // If we didn't pick up a folder from the heuristic above for whatever reason, use root
  if (folder == nullptr) {
    folder = project()->root();
  }

  return folder;
}

ProjectViewModel *ProjectExplorer::model()
{
  return &model_;
}

void ProjectExplorer::SelectAll()
{
  CurrentView()->selectAll();
}

void ProjectExplorer::DeselectAll()
{
  CurrentView()->selectionModel()->clearSelection();
}

void ProjectExplorer::DeleteSelected()
{
  QVector<Node*> selected = SelectedItems();

  if (selected.isEmpty()) {
    return;
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  bool dont_confirm_footage_in_use = false;

  foreach (Node* item, selected) {
    // Verify whether this item is in use anywhere
    Footage* footage_cast_test = dynamic_cast<Footage*>(item);
    Sequence* sequence_cast_test = dynamic_cast<Sequence*>(item);

    bool cleared_to_delete = true;

    if (sequence_cast_test) {
      if (Core::instance()->main_window()->IsSequenceOpen(sequence_cast_test)) {
        Core::instance()->main_window()->CloseSequence(sequence_cast_test);
      }
    } else if (footage_cast_test) {
      if (!footage_cast_test->output_connections().empty() && !dont_confirm_footage_in_use) {
        // Footage outputs to other nodes, warn the user
        QMessageBox msgbox(this);
        msgbox.setWindowTitle(tr("Confirm Footage Deletion"));
        msgbox.setIcon(QMessageBox::Warning);

        QStringList connected_nodes;
        foreach (const Node::OutputConnection& c, footage_cast_test->output_connections()) {
          Node* connected = c.second.node();

          if (connected->GetLabel().isEmpty()) {
            connected_nodes.append(connected->Name());
          } else {
            connected_nodes.append(QStringLiteral("%1 (%2)").arg(connected->GetLabel(), connected->Name()));
          }
        }

        msgbox.setText(tr("The footage \"%1\" is currently connected to the following nodes:\n\n"
                          "%2\n\n"
                          "Are you sure you wish to delete this footage?")
                       .arg(footage_cast_test->filename(), connected_nodes.join('\n')));


        // Set up buttons
        msgbox.addButton(QMessageBox::Yes);
        msgbox.addButton(QMessageBox::YesToAll);
        msgbox.addButton(QMessageBox::No);
        msgbox.addButton(QMessageBox::Cancel);

        // Run messagebox
        int r = msgbox.exec();

        switch (r) {
        case QMessageBox::Cancel:
          // Stop this entire function
          delete command;
          return;
        case QMessageBox::No:
          cleared_to_delete = false;
          break;
        case QMessageBox::YesToAll:
          dont_confirm_footage_in_use = true;
          break;
        }
      }

      if (cleared_to_delete) {
        // Close footage if currently open in footage panel
        FootageViewerPanel* footage_panel = PanelManager::instance()->GetPanelsOfType<FootageViewerPanel>().first();
        if (footage_panel->GetSelectedFootage().contains(footage_cast_test)) {
          footage_panel->SetFootage(nullptr);
        }
      }
    }

    if (cleared_to_delete) {
      command->add_child(new NodeRemoveAndDisconnectCommand(reinterpret_cast<Node*>(item)));
    }
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

}
