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
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QVBoxLayout>

#include "common/define.h"
#include "core.h"
#include "dialog/footageproperties/footageproperties.h"
#include "dialog/sequence/sequence.h"
#include "task/proxy/proxy.h"
#include "task/taskmanager.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

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

  // Add tree view to stacked widget
  tree_view_ = new ProjectExplorerTreeView(stacked_widget_);
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

void ProjectExplorer::Edit(Item *item)
{
  CurrentView()->edit(model_.CreateIndexFromItem(item));
}

void ProjectExplorer::AddView(QAbstractItemView *view)
{
  view->setModel(&model_);
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
    Folder* f = static_cast<Folder*>(index.internalPointer());
    nav_bar_->set_text(f->name());
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
  Item* i = static_cast<Item*>(index.internalPointer());

  // If the item is a folder, browse to it
  if (i->CanHaveChildren()
      && (view_type() == ProjectToolbar::ListView || view_type() == ProjectToolbar::IconView)) {

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

  // FIXME: Support for multiple items and items other than Footage
  QList<Item*> selected_items = SelectedItems();

  if (selected_items.isEmpty()) {
    new_menu.setTitle(tr("&New"));
    MenuShared::instance()->AddItemsForNewMenu(&new_menu);
    menu.addMenu(&new_menu);

    menu.addSeparator();

    // FIXME: These are both duplicates of items from MainMenu, is there any way to re-use the code?
    QAction* import_action = menu.addAction(tr("&Import..."));
    connect(import_action, &QAction::triggered, Core::instance(), &Core::DialogImportShow);

    menu.addSeparator();

    QAction* project_properties = menu.addAction(tr("&Project Properties..."));
    connect(project_properties, &QAction::triggered, Core::instance(), &Core::DialogProjectPropertiesShow);
  } else {
    context_menu_item_ = selected_items.first();

    if (context_menu_item_->type() == Item::kFolder) {

      QAction* open_in_new_tab = menu.addAction(tr("Open in New Tab"));
      connect(open_in_new_tab, &QAction::triggered, this, &ProjectExplorer::OpenContextMenuItemInNewTab);

      QAction* open_in_new_window = menu.addAction(tr("Open in New Window"));
      connect(open_in_new_window, &QAction::triggered, this, &ProjectExplorer::OpenContextMenuItemInNewWindow);

      menu.addSeparator();

    } else if (context_menu_item_->type() == Item::kFootage) {
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

      menu.addSeparator();

      Footage* f = static_cast<Footage*>(context_menu_item_);

      if (f->HasStreamsOfType(Stream::kVideo)) {
        Menu* proxy_menu = new Menu(tr("Proxy"), &menu);
        menu.addMenu(proxy_menu);

        VideoStreamPtr video_stream = std::static_pointer_cast<VideoStream>(f->get_first_stream_of_type(Stream::kVideo));

        if (video_stream->is_generating_proxy()) {

          // Prevent multiple proxy actions from occurring at once
          QAction* cant_proxy_action = proxy_menu->addAction(tr("Proxy being generated..."));
          cant_proxy_action->setEnabled(false);

        } else {

          proxy_menu->addAction(tr("(None)"))->setData(0);
          proxy_menu->addSeparator();
          proxy_menu->addAction(tr("Full"))->setData(1);
          proxy_menu->addAction(tr("1/2"))->setData(2);
          proxy_menu->addAction(tr("1/4"))->setData(4);
          proxy_menu->addAction(tr("1/8"))->setData(8);

          foreach (QAction* a, proxy_menu->actions()) {
            a->setCheckable(true);

            if (a->data() == video_stream->using_proxy()) {
              a->setChecked(true);
            }
          }

          connect(proxy_menu, &Menu::triggered, this, &ProjectExplorer::ContextMenuStartProxy);

        }

        menu.addSeparator();
      }
    }

    QAction* properties_action = menu.addAction(tr("P&roperties"));

    if (context_menu_item_->type() == Item::kFootage) {
      connect(properties_action, &QAction::triggered, this, &ProjectExplorer::ShowFootagePropertiesDialog);
    } else if (context_menu_item_->type() == Item::kSequence) {
      connect(properties_action, &QAction::triggered, this, &ProjectExplorer::ShowSequencePropertiesDialog);
    }
  }

  menu.exec(QCursor::pos());
}

void ProjectExplorer::ShowFootagePropertiesDialog()
{
  // FIXME: Support for multiple items
  FootagePropertiesDialog fpd(this, static_cast<Footage*>(context_menu_item_));
  fpd.exec();
}

void ProjectExplorer::ShowSequencePropertiesDialog()
{
  // FIXME: Support for multiple items
  SequenceDialog sd(static_cast<Sequence*>(context_menu_item_), SequenceDialog::kExisting, this);
  sd.exec();
}

void ProjectExplorer::RevealSelectedFootage()
{
  Footage* footage = static_cast<Footage*>(context_menu_item_);

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
  Core::instance()->main_window()->FolderOpen(project(), context_menu_item_, false);
}

void ProjectExplorer::OpenContextMenuItemInNewWindow()
{
  Core::instance()->main_window()->FolderOpen(project(), context_menu_item_, true);
}

void ProjectExplorer::ContextMenuStartProxy(QAction *a)
{
  QList<Item*> selected = SelectedItems();
  foreach (Item* item, selected) {
    // Not sure if this is needed but seems safer to have
    if (item->type() != Footage::kFootage) {
      continue;
    }

    // Find video stream
    VideoStreamPtr video_stream = nullptr;

    foreach (StreamPtr s, static_cast<Footage*>(item)->streams()) {
      if (s->type() == Stream::kVideo) {
        video_stream = std::static_pointer_cast<VideoStream>(s);
        break;
      }
    }

    if (!video_stream) {
      return;
    }

    int chosen_proxy_setting = a->data().toInt();

    if (chosen_proxy_setting != video_stream->using_proxy()) {
      if (!a->data().toInt()) {
        // 0 means disable the proxy
         video_stream->set_proxy(0, QVector<int64_t>());

      } else if (video_stream->try_start_proxy()) {
        // Start a background task for proxying
         ProxyTask* proxy_task = new ProxyTask(video_stream, a->data().toInt());
         TaskManager::instance()->AddTask(proxy_task);
      }
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

void ProjectExplorer::set_root(Item *item)
{
  QModelIndex index = model_.CreateIndexFromItem(item);

  BrowseToFolder(index);
  tree_view_->setRootIndex(index);
}

QList<Item *> ProjectExplorer::SelectedItems() const
{
  // Determine which view is active and get its selected indexes
  QModelIndexList index_list = CurrentView()->selectionModel()->selectedRows();

  // Convert indexes to item objects
  QList<Item*> selected_items;

  for (int i=0;i<index_list.size();i++) {
    const QModelIndex& index = index_list.at(i);

    Item* item = static_cast<Item*>(index.internalPointer());

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
  QList<Item*> selected_items = SelectedItems();

  // Heuristic for finding the selected folder:
  //
  // - If `folder` is nullptr, we set the first folder we find. Either the item itself if it's a folder, or the
  //   item's parent.
  // - Otherwise, if all folders found are the same, we'll use that to import into.
  // - If more than one folder is found, we play it safe and import into the root folder

  for (int i=0;i<selected_items.size();i++) {
    Item* sel_item = selected_items.at(i);

    // If this item is not a folder, presumably it's parent is
    if (!sel_item->CanHaveChildren()) {
      sel_item = sel_item->parent();

      Q_ASSERT(sel_item->CanHaveChildren());
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
  QList<Item*> selected = SelectedItems();

  if (selected.isEmpty()) {
    return;
  }

  QUndoCommand* command = new QUndoCommand();

  foreach (Item* item, selected) {
    ItemPtr item_ptr = item->get_shared_ptr();

    // If this is a sequence, close it
    if (item_ptr->type() == Item::kSequence) {
      Sequence* s = static_cast<Sequence*>(item_ptr.get());

      if (Core::instance()->main_window()->IsSequenceOpen(s)) {
        Core::instance()->main_window()->CloseSequence(s);
      }
    }

    new ProjectViewModel::RemoveItemCommand(&model_, item_ptr, command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);
}

OLIVE_NAMESPACE_EXIT
