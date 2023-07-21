/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include <QVBoxLayout>

#include "common/define.h"
#include "core.h"
#include "dialog/footageproperties/footageproperties.h"
#include "dialog/sequence/sequence.h"
#include "projectexplorerundo.h"
#include "task/precache/precachetask.h"
#include "task/taskmanager.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "node/nodeundo.h"
#include "window/mainwindow/mainwindow.h"
#include "window/mainwindow/mainwindowundo.h"
#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QWidget(parent),
  model_(this)
{
  // Create layout
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

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
  sort_model_.setFilterCaseSensitivity(Qt::CaseInsensitive);
  sort_model_.setSortRole(ProjectViewModel::kInnerTextRole);

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

  connect(tree_view_, &ProjectExplorerTreeView::customContextMenuRequested, this, &ProjectExplorer::ShowContextMenu);
  connect(list_view_, &ProjectExplorerListView::customContextMenuRequested, this, &ProjectExplorer::ShowContextMenu);
  connect(icon_view_, &ProjectExplorerIconView::customContextMenuRequested, this, &ProjectExplorer::ShowContextMenu);

  UpdateNavBarText();
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
  view->setEditTriggers(QAbstractItemView::SelectedClicked);
  connect(view, &QAbstractItemView::doubleClicked, this, &ProjectExplorer::ItemDoubleClickedSlot);
  connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProjectExplorer::ViewSelectionChanged);
  connect(view, SIGNAL(DoubleClickedEmptyArea()), this, SLOT(ViewEmptyAreaDoubleClickedSlot()));
  stacked_widget_->addWidget(view);
}

void ProjectExplorer::BrowseToFolder(const QModelIndex &index)
{
  // Set appropriate views to this index
  icon_view_->setRootIndex(index);
  list_view_->setRootIndex(index);

  // Set navbar text to folder's name
  UpdateNavBarText();

  // Set directory up enabled button based on whether we're in root or not
  nav_bar_->set_dir_up_enabled(index.isValid());
}

int ProjectExplorer::ConfirmItemDeletion(Node* item)
{
  QMessageBox msgbox(this);
  msgbox.setWindowTitle(tr("Confirm Item Deletion"));
  msgbox.setIcon(QMessageBox::Warning);

  QStringList connected_nodes_names;
  foreach (const Node::OutputConnection& connected, item->output_connections()) {
    if (!dynamic_cast<Folder*>(connected.second.node())) {
      connected_nodes_names.append(GetHumanReadableNodeName(connected.second.node()));
    }
  }

  msgbox.setText(tr("The item \"%1\" is currently connected to the following nodes:\n\n"
                    "%2\n\n"
                    "Are you sure you wish to delete this footage?")
                 .arg(GetHumanReadableNodeName(item), connected_nodes_names.join('\n')));

  // Set up buttons
  msgbox.addButton(QMessageBox::Yes);
  msgbox.addButton(QMessageBox::YesToAll);
  msgbox.addButton(QMessageBox::No);
  msgbox.addButton(QMessageBox::Cancel);

  // Run messagebox
  return msgbox.exec();
}

bool ProjectExplorer::DeleteItemsInternal(const QVector<Node*>& selected, bool& check_if_item_is_in_use, MultiUndoCommand* command)
{
  for (int i=0; i<selected.size(); i++) {
    // Delete sequences first
    Node* node = selected.at(i);

    bool can_delete_item = true;

    if (check_if_item_is_in_use) {
      foreach (const Node::OutputConnection& oc, node->output_connections()) {
        Folder* folder_test = dynamic_cast<Folder*>(oc.second.node());
        if (!folder_test) {
          // This sequence outputs to SOMETHING, confirm the user if they want to delete this
          int r = ConfirmItemDeletion(node);

          switch (r) {
          case QMessageBox::No:
            can_delete_item = false;
            break;
          case QMessageBox::Cancel:
            return false;
          case QMessageBox::YesToAll:
            check_if_item_is_in_use = false;
            break;
          }
        }
      }
    }

    if (can_delete_item) {
      Sequence* sequence = dynamic_cast<Sequence*>(node);
      if (sequence && Core::instance()->main_window()->IsSequenceOpen(sequence)) {
        command->add_child(new CloseSequenceCommand(sequence));
      }

      if (node->folder()) {
        command->add_child(new Folder::RemoveElementCommand(node->folder(), node));
      }

      command->add_child(new NodeRemoveWithExclusiveDependenciesAndDisconnect(node));
    }
  }

  return true;
}

QString ProjectExplorer::GetHumanReadableNodeName(Node *node)
{
  if (node->GetLabel().isEmpty()) {
    return node->Name();
  } else {
    return tr("%1 (%2)").arg(node->GetLabel(), node->Name());
  }
}

void ProjectExplorer::UpdateNavBarText()
{
  QString absolute;

  Folder* f = static_cast<Folder*>(sort_model_.mapToSource(list_view_->rootIndex()).internalPointer());
  while (f && f != project()->root()) {
    absolute.prepend(QStringLiteral("%1 / ").arg(f->GetLabel()));
    f = f->folder();
  }

  absolute.prepend(QStringLiteral("/ "));

  nav_bar_->set_text(absolute);
}

QAbstractItemView *ProjectExplorer::CurrentView() const
{
  return static_cast<QAbstractItemView*>(stacked_widget_->currentWidget());
}

void ProjectExplorer::ViewEmptyAreaDoubleClickedSlot()
{
  emit DoubleClickedItem(nullptr);
}

void ProjectExplorer::ItemDoubleClickedSlot(const QModelIndex &index)
{
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

void ProjectExplorer::RenameSelectedItem()
{
  auto indexes = CurrentView()->selectionModel()->selectedRows();
  if (!indexes.empty()) {
    CurrentView()->edit(indexes.first());
  }
}

void ProjectExplorer::SetSearchFilter(const QString &s)
{
  sort_model_.setFilterFixedString(s);
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

        QAction *replace_action = menu.addAction(tr("Replace Footage"));
        connect(replace_action, &QAction::triggered, this, &ProjectExplorer::ReplaceSelectedFootage);

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
          a->setData(QtUtils::PtrToValue(i));
        }

        connect(proxy_menu, &Menu::triggered, this, &ProjectExplorer::ContextMenuStartProxy);
      }
    }

    Q_UNUSED(all_items_are_footage_or_sequence)

    if (context_menu_items_.size() == 1) {
      menu.addSeparator();

      auto rename_action = menu.addAction(tr("Rename"));
      connect(rename_action, &QAction::triggered, this, &ProjectExplorer::RenameSelectedItem);
    }

    auto delete_action = menu.addAction(tr("Delete"));
    connect(delete_action, &QAction::triggered, this, &ProjectExplorer::DeleteSelected);

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

    FootagePropertiesDialog fpd(this, static_cast<Footage*>(sel));
    fpd.exec();

  } else if (dynamic_cast<Folder*>(sel)) {

    Core::instance()->LabelNodes(context_menu_items_);

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

void ProjectExplorer::ReplaceSelectedFootage()
{
  Footage* footage = static_cast<Footage*>(context_menu_items_.first());

  QString file = QFileDialog::getOpenFileName(this, tr("Replace Footage"));
  if (!file.isEmpty()) {
    auto p = new MultiUndoCommand();

    // Change filename parameter
    p->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(NodeInput(footage, Footage::kFilenameInput)), file));

    if (QFileInfo(footage->filename()).fileName() == footage->GetLabel()) {
      // Footage label == filename, change label too
      p->add_child(new NodeRenameCommand(footage, QFileInfo(file).fileName()));
    }

    Core::instance()->undo_stack()->push(p, tr("Replaced Footage"));
  }
}

void ProjectExplorer::OpenContextMenuItemInNewTab()
{
  Core::instance()->main_window()->OpenFolder(static_cast<Folder*>(context_menu_items_.first()), false);
}

void ProjectExplorer::OpenContextMenuItemInNewWindow()
{
  Core::instance()->main_window()->OpenFolder(static_cast<Folder*>(context_menu_items_.first()), true);
}

void ProjectExplorer::ContextMenuStartProxy(QAction *a)
{
  Sequence* sequence = QtUtils::ValueToPtr<Sequence>(a->data());

  // To get here, the `context_menu_items_` must be all kFootage
  foreach (Node* item, context_menu_items_) {
    Footage* f = static_cast<Footage*>(item);

    int sz = f->InputArraySize(Footage::kVideoParamsInput);

    for (int j=0; j<sz; j++) {
      VideoParams vp = f->GetVideoParams(j);

      if (vp.enabled()) {
        // Start a background task for proxying
        PreCacheTask* proxy_task = new PreCacheTask(f, j, sequence);
        TaskManager::instance()->AddTask(proxy_task);
      }
    }
  }
}

void ProjectExplorer::ViewSelectionChanged()
{
  QItemSelectionModel *model = static_cast<QItemSelectionModel *>(sender());

  QModelIndexList selection = model->selectedIndexes();

  QVector<Node *> nodes;

  foreach (const QModelIndex &index, selection) {
    Node *sel = static_cast<Node*>(sort_model_.mapToSource(index).internalPointer());
    if (!nodes.contains(sel)) {
      nodes.append(sel);
    }
  }

  if (nodes.isEmpty()) {
    nodes.append(get_root());
  }

  emit SelectionChanged(nodes);
}

Project *ProjectExplorer::project() const
{
  return model_.project();
}

void ProjectExplorer::set_project(Project *p)
{
  model_.set_project(p);
}

Folder *ProjectExplorer::get_root() const
{
  QModelIndex root_index = sort_model_.mapToSource(tree_view_->rootIndex());

  if (!root_index.isValid()) {
    return project()->root();
  }

  return static_cast<Folder *>(root_index.internalPointer());
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

  bool check_if_item_is_in_use = true;

  if (DeleteItemsInternal(selected, check_if_item_is_in_use, command)) {
    Core::instance()->undo_stack()->push(command, tr("Deleted %1 Item(s)").arg(selected.size()));
  } else {
    delete command;
  }
}

bool ProjectExplorer::SelectItem(Node *n, bool deselect_all_first)
{
  if (deselect_all_first) {
    DeselectAll();
  }

  QModelIndex index = model_.CreateIndexFromItem(n);

  if (index.isValid()) {
    index = sort_model_.mapFromSource(index);

    QModelIndex parent = index.parent();
    if (view_type() == ProjectToolbar::TreeView) {
      // Expand all folders until this index is visible
      while (parent.isValid()) {
        tree_view_->expand(parent);
        parent = parent.parent();
      }
    } else {
      BrowseToFolder(parent);
    }

    CurrentView()->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);

    return true;
  }

  return false;
}

}
