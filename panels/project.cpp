/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QApplication>
#include <QFileDialog>
#include <QString>
#include <QVariant>
#include <QCharRef>
#include <QMessageBox>
#include <QDropEvent>
#include <QMimeData>
#include <QPushButton>
#include <QInputDialog>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSizePolicy>
#include <QVBoxLayout>

#include "global/global.h"
#include "panels.h"
#include "rendering/renderfunctions.h"
#include "project/previewgenerator.h"
#include "undo/undo.h"
#include "undo/undostack.h"
#include "ui/mainwindow.h"
#include "global/config.h"
#include "rendering/cacher.h"
#include "dialogs/replaceclipmediadialog.h"
#include "panels/effectcontrols.h"
#include "dialogs/mediapropertiesdialog.h"
#include "dialogs/newsequencedialog.h"
#include "dialogs/loaddialog.h"
#include "project/clipboard.h"
#include "ui/sourcetable.h"
#include "ui/sourceiconview.h"
#include "ui/icons.h"
#include "ui/menuhelper.h"
#include "ui/mediaiconservice.h"
#include "project/sourcescommon.h"
#include "project/projectfilter.h"
#include "project/projectfunctions.h"
#include "global/debug.h"
#include "ui/menu.h"

Project::Project(QWidget *parent) :
  Panel(parent),
  sorter(this),
  sources_common(this, sorter)
{
  QWidget* dockWidgetContents = new QWidget(this);

  QVBoxLayout* verticalLayout = new QVBoxLayout(dockWidgetContents);
  verticalLayout->setMargin(0);
  verticalLayout->setSpacing(0);

  setWidget(dockWidgetContents);

  ConnectFilterToModel();

  // optional toolbar
  toolbar_widget = new QWidget();
  toolbar_widget->setVisible(olive::CurrentConfig.show_project_toolbar);
  toolbar_widget->setObjectName("project_toolbar");

  QHBoxLayout* toolbar = new QHBoxLayout(toolbar_widget);
  toolbar->setMargin(0);
  toolbar->setSpacing(0);

  QPushButton* toolbar_new = new QPushButton();
  toolbar_new->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/add-button.svg")));
  toolbar_new->setToolTip(tr("New"));
  connect(toolbar_new, SIGNAL(clicked(bool)), this, SLOT(make_new_menu()));
  toolbar->addWidget(toolbar_new);

  QPushButton* toolbar_open = new QPushButton();
  toolbar_open->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/open.svg")));
  toolbar_open->setToolTip(tr("Open Project"));
  connect(toolbar_open, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(OpenProject()));
  toolbar->addWidget(toolbar_open);

  QPushButton* toolbar_save = new QPushButton();
  toolbar_save->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/save.svg")));
  toolbar_save->setToolTip(tr("Save Project"));
  connect(toolbar_save, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(save_project()));
  toolbar->addWidget(toolbar_save);

  QPushButton* toolbar_undo = new QPushButton();
  toolbar_undo->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/undo.svg")));
  toolbar_undo->setToolTip(tr("Undo"));
  connect(toolbar_undo, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(undo()));
  toolbar->addWidget(toolbar_undo);

  QPushButton* toolbar_redo = new QPushButton();
  toolbar_redo->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/redo.svg")));
  toolbar_redo->setToolTip(tr("Redo"));
  connect(toolbar_redo, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(redo()));
  toolbar->addWidget(toolbar_redo);

  toolbar_search = new QLineEdit();
  toolbar_search->setClearButtonEnabled(true);
  connect(toolbar_search, SIGNAL(textChanged(QString)), &sorter, SLOT(update_search_filter(const QString&)));
  toolbar->addWidget(toolbar_search);

  QPushButton* toolbar_tree_view = new QPushButton();
  toolbar_tree_view->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/treeview.svg")));
  toolbar_tree_view->setToolTip(tr("Tree View"));
  connect(toolbar_tree_view, SIGNAL(clicked(bool)), this, SLOT(set_tree_view()));
  toolbar->addWidget(toolbar_tree_view);

  QPushButton* toolbar_icon_view = new QPushButton();
  toolbar_icon_view->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/iconview.svg")));
  toolbar_icon_view->setToolTip(tr("Icon View"));
  connect(toolbar_icon_view, SIGNAL(clicked(bool)), this, SLOT(set_icon_view()));
  toolbar->addWidget(toolbar_icon_view);

  QPushButton* toolbar_list_view = new QPushButton();
  toolbar_list_view->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/listview.svg")));
  toolbar_list_view->setToolTip(tr("List View"));
  connect(toolbar_list_view, SIGNAL(clicked(bool)), this, SLOT(set_list_view()));
  toolbar->addWidget(toolbar_list_view);

  verticalLayout->addWidget(toolbar_widget);

  // tree view
  tree_view = new SourceTable(sources_common);
  tree_view->project_parent = this;
  tree_view->setModel(&sorter);
  verticalLayout->addWidget(tree_view);

  // Set the first column width
  // I'm not sure if there's a better way to do this, default behavior seems to have all columns fixed width
  // and let the last column fill up the remainder when really the opposite would be preferable (having the
  // first column fill up the majority of the space). Anyway, this will probably do for now.
  tree_view->setColumnWidth(0, tree_view->width()/2);

  // icon view
  icon_view_container = new QWidget();

  QVBoxLayout* icon_view_container_layout = new QVBoxLayout(icon_view_container);
  icon_view_container_layout->setMargin(0);
  icon_view_container_layout->setSpacing(0);

  QHBoxLayout* icon_view_controls = new QHBoxLayout();
  icon_view_controls->setMargin(0);
  icon_view_controls->setSpacing(0);

  directory_up = new QPushButton();
  directory_up->setIcon(olive::icon::CreateIconFromSVG(QStringLiteral(":/icons/dirup.svg")));
  directory_up->setEnabled(false);
  icon_view_controls->addWidget(directory_up);

  icon_view_controls->addStretch();

  icon_size_slider = new QSlider(Qt::Horizontal);
  icon_size_slider->setMinimum(16);
  icon_size_slider->setMaximum(256);
  icon_view_controls->addWidget(icon_size_slider);
  connect(icon_size_slider, SIGNAL(valueChanged(int)), this, SLOT(set_icon_view_size(int)));

  icon_view_container_layout->addLayout(icon_view_controls);

  icon_view = new SourceIconView(sources_common);
  icon_view->project_parent = this;
  icon_view->setModel(&sorter);
  icon_view->setGridSize(QSize(100, 100));
  icon_view->setViewMode(QListView::IconMode);
  icon_view->setUniformItemSizes(true);
  icon_view_container_layout->addWidget(icon_view);

  icon_size_slider->setValue(icon_view->gridSize().height());

  verticalLayout->addWidget(icon_view_container);

  connect(directory_up, SIGNAL(clicked(bool)), this, SLOT(go_up_dir()));
  connect(icon_view, SIGNAL(changed_root()), this, SLOT(set_up_dir_enabled()));

  connect(olive::media_icon_service.get(), SIGNAL(IconChanged()), icon_view->viewport(), SLOT(update()));
  connect(olive::media_icon_service.get(), SIGNAL(IconChanged()), tree_view->viewport(), SLOT(update()));

  update_view_type();

  Retranslate();
}

void Project::ConnectFilterToModel()
{
  sorter.setSourceModel(&olive::project_model);
}

void Project::DisconnectFilterToModel()
{
  sorter.setSourceModel(nullptr);
}

void Project::Retranslate() {
  toolbar_search->setPlaceholderText(tr("Search media, markers, etc."));
  setWindowTitle(tr("Project"));
}

void Project::duplicate_selected() {
  QModelIndexList items = get_current_selected();
  bool duped = false;
  ComboAction* ca = new ComboAction();
  for (int j=0;j<items.size();j++) {
    Media* i = item_to_media(items.at(j));
    if (i->get_type() == MEDIA_TYPE_SEQUENCE) {
      olive::project_model.CreateSequence(ca, i->to_sequence()->copy(), false, item_to_media(items.at(j).parent()));
      duped = true;
    }
  }
  if (duped) {
    olive::UndoStack.push(ca);
  } else {
    delete ca;
  }
}

void Project::replace_selected_file() {
  QModelIndexList selected_items = get_current_selected();
  if (selected_items.size() == 1) {
    MediaPtr item = item_to_media_ptr(selected_items.at(0));
    if (item->get_type() == MEDIA_TYPE_FOOTAGE) {
      sources_common.replace_media(item, nullptr);
    }
  }
}

void Project::replace_clip_media() {
  if (olive::ActiveSequence == nullptr) {
    QMessageBox::critical(this,
                          tr("No active sequence"),
                          tr("No sequence is active, please open the sequence you want to replace clips from."),
                          QMessageBox::Ok);
  } else {
    QModelIndexList selected_items = get_current_selected();
    if (selected_items.size() == 1) {
      Media* item = item_to_media(selected_items.at(0));
      if (item->get_type() == MEDIA_TYPE_SEQUENCE && olive::ActiveSequence == item->to_sequence()) {
        QMessageBox::critical(this,
                              tr("Active sequence selected"),
                              tr("You cannot insert a sequence into itself, so no clips of this media would be in this sequence."),
                              QMessageBox::Ok);
      } else {
        ReplaceClipMediaDialog dialog(this, item);
        dialog.exec();
      }
    }
  }
}

void Project::open_properties() {
  QModelIndexList selected_items = get_current_selected();
  if (selected_items.size() == 1) {
    Media* item = item_to_media(selected_items.at(0));
    switch (item->get_type()) {
    case MEDIA_TYPE_FOOTAGE:
    {
      MediaPropertiesDialog mpd(this, item);
      mpd.exec();
    }
      break;
    case MEDIA_TYPE_SEQUENCE:
    {
      NewSequenceDialog nsd(this, item);
      nsd.exec();
    }
      break;
    default:
    {
      // fall back to renaming
      QString new_name = QInputDialog::getText(this,
                                               tr("Rename '%1'").arg(item->get_name()),
                                               tr("Enter new name:"),
                                               QLineEdit::Normal,
                                               item->get_name());
      if (!new_name.isEmpty()) {
        MediaRename* mr = new MediaRename(item, new_name);
        olive::UndoStack.push(mr);
      }
    }
    }
  }
}

void Project::new_folder() {
  MediaPtr m = olive::project::CreateFolder(nullptr);
  olive::UndoStack.push(new AddMediaCommand(m, get_selected_folder()));

  QModelIndex index = olive::project_model.create_index(m->row(), 0, m.get());
  switch (olive::CurrentConfig.project_view_type) {
  case olive::PROJECT_VIEW_TREE:
    tree_view->edit(sorter.mapFromSource(index));
    break;
  case olive::PROJECT_VIEW_ICON:
    icon_view->edit(sorter.mapFromSource(index));
    break;
  }
}

bool Project::focused() {
  return tree_view->hasFocus() || icon_view->hasFocus();
}

Media* Project::item_to_media(const QModelIndex &index) {
  return static_cast<Media*>(sorter.mapToSource(index).internalPointer());
}

MediaPtr Project::item_to_media_ptr(const QModelIndex &index) {
  Media* raw_ptr = item_to_media(index);

  if (raw_ptr == nullptr) {
    return nullptr;
  }

  return raw_ptr->parentItem()->get_shared_ptr(raw_ptr);
}

bool Project::IsToolbarVisible()
{
  return toolbar_widget->isVisible();
}

void Project::SetToolbarVisible(bool visible)
{
  toolbar_widget->setVisible(visible);
}

bool Project::IsProjectWidget(QObject *child)
{
  return (child == tree_view || child == icon_view);
}

bool delete_clips_in_clipboard_with_media(ComboAction* ca, Media* m) {
  int delete_count = 0;
  if (clipboard_type == CLIPBOARD_TYPE_CLIP) {
    for (int i=0;i<clipboard.size();i++) {
      ClipPtr c = std::static_pointer_cast<Clip>(clipboard.at(i));
      if (c->media() == m) {
        ca->append(new RemoveClipsFromClipboard(i-delete_count));
        delete_count++;
      }
    }
  }
  return (delete_count > 0);
}

void Project::delete_selected_media() {
  ComboAction* ca = new ComboAction();
  QModelIndexList selected_items = get_current_selected();

  QList<Media*> items;
  for (int i=0;i<selected_items.size();i++) {
    items.append(item_to_media(selected_items.at(i)));
  }

  bool remove = true;
  bool redraw = false;

  // check if media is in use
  QVector<Media*> parents;
  QVector<Media*> all_sequences = olive::project_model.GetAllSequences();
  if (all_sequences.size() > 0) {

    QVector<Media*> all_footage = olive::project_model.GetAllFootage();

    for (int i=0;i<all_footage.size();i++) {
      Media* item = all_footage.at(i);
      Footage* media = item->to_footage();
      bool confirm_delete = false;
      for (int j=0;j<all_sequences.size();j++) {

        Sequence* s = all_sequences.at(j)->to_sequence().get();
        QVector<Clip*> sequence_clips = s->GetAllClips();

        for (int k=0;k<sequence_clips.size();k++) {
          Clip* c = sequence_clips.at(k);
          if (c != nullptr && c->media() == item) {
            if (!confirm_delete) {
              // we found a reference, so we know we'll need to ask if the user wants to delete it
              QMessageBox confirm(this);
              confirm.setWindowTitle(tr("Delete media in use?"));
              confirm.setText(tr("The media '%1' is currently used in '%2'. Deleting it will remove all instances in the sequence. Are you sure you want to do this?").arg(media->name, s->name));
              QAbstractButton* yes_button = confirm.addButton(QMessageBox::Yes);
              QAbstractButton* skip_button = nullptr;
              if (items.size() > 1) skip_button = confirm.addButton(tr("Skip"), QMessageBox::NoRole);
              QAbstractButton* abort_button = confirm.addButton(QMessageBox::Cancel);
              confirm.exec();
              if (confirm.clickedButton() == yes_button) {
                // remove all clips referencing this media
                confirm_delete = true;
                redraw = true;
              } else if (confirm.clickedButton() == skip_button) {
                // remove media item and any folders containing it from the remove list
                Media* parent = item;
                while (parent != nullptr) {
                  parents.append(parent);

                  // re-add item's siblings
                  for (int m=0;m<parent->childCount();m++) {
                    Media* child = parent->child(m);
                    bool found = false;
                    for (int n=0;n<items.size();n++) {
                      if (items.at(n) == child) {
                        found = true;
                        break;
                      }
                    }
                    if (!found) {
                      items.append(child);
                    }
                  }

                  parent = parent->parentItem();
                }

                j = all_sequences.size();
                k = sequence_clips.size();
              } else if (confirm.clickedButton() == abort_button) {
                // break out of loop
                i = all_footage.size();
                j = all_sequences.size();
                k = sequence_clips.size();

                remove = false;
              }
            }
            if (confirm_delete) {
              ca->append(new DeleteClipAction(c));
            }
          }
        }
      }
      if (confirm_delete) {
        delete_clips_in_clipboard_with_media(ca, item);
      }
    }
  }

  // remove
  if (remove) {
    panel_graph_editor->set_row(nullptr);
    panel_effect_controls->Clear(true);

    if (olive::ActiveSequence != nullptr) {
      olive::ActiveSequence->ClearSelections();
    }

    // remove media and parents
    for (int m=0;m<parents.size();m++) {
      for (int l=0;l<items.size();l++) {
        if (items.at(l) == parents.at(m)) {
          items.removeAt(l);
          l--;
        }
      }
    }

    for (int i=0;i<items.size();i++) {

      ca->append(new DeleteMediaCommand(items.at(i)->parentItem()->get_shared_ptr(items.at(i))));

      if (items.at(i)->get_type() == MEDIA_TYPE_SEQUENCE) {
        redraw = true;

        Sequence* s = items.at(i)->to_sequence().get();

        if (s == olive::ActiveSequence.get()) {
          ca->append(new ChangeSequenceAction(nullptr));
        }

        if (s == panel_footage_viewer->seq.get()) {
          panel_footage_viewer->set_media(nullptr);
        }
      } else if (items.at(i)->get_type() == MEDIA_TYPE_FOOTAGE) {
        if (panel_footage_viewer->media == items.at(i)) {
          panel_footage_viewer->set_media(nullptr);
        }
      }
    }
    olive::UndoStack.push(ca);

    // redraw clips
    if (redraw) {
      update_ui(true);
    }
  } else {
    delete ca;
  }
}

Media* Project::get_selected_folder() {
  // if one item is selected and it's a folder, return it
  QModelIndexList selected_items = get_current_selected();
  if (selected_items.size() == 1) {
    Media* m = item_to_media(selected_items.at(0));
    if (m->get_type() == MEDIA_TYPE_FOLDER) return m;
  }
  return nullptr;
}

bool Project::reveal_media(Media *media, QModelIndex parent) {
  for (int i=0;i<olive::project_model.rowCount(parent);i++) {
    const QModelIndex& item = olive::project_model.index(i, 0, parent);
    Media* m = olive::project_model.getItem(item);

    if (m->get_type() == MEDIA_TYPE_FOLDER) {

      // if this item is a folder, recursively run this function to search it too
      if (reveal_media(media, item)) return true;

    } else if (m == media) {
      // if m == media, then we found the media object we were looking for

      // get sorter proxy item (the item that's "visible")
      QModelIndex sorted_index = sorter.mapFromSource(item);

      // retrieve its parent item
      QModelIndex hierarchy = sorted_index.parent();

      if (olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_TREE) {

        // if we're in tree view, expand every folder in the hierarchy containing the media
        while (hierarchy.isValid()) {
          tree_view->setExpanded(hierarchy, true);
          hierarchy = hierarchy.parent();
        }

        // select item (requires a QItemSelection object to select the whole row)
        QItemSelection row_select(
              sorter.index(sorted_index.row(), 0, sorted_index.parent()),
              sorter.index(sorted_index.row(), sorter.columnCount()-1, sorted_index.parent())
              );

        tree_view->selectionModel()->select(row_select, QItemSelectionModel::Select);
      } else if (olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_ICON) {

        // if we're in icon view, we just "browse" to the parent folder
        icon_view->setRootIndex(hierarchy);

        // select item in this folder
        icon_view->selectionModel()->select(sorted_index, QItemSelectionModel::Select);

        // update the "up" button state
        set_up_dir_enabled();

      }

      return true;
    }
  }

  return false;
}

void Project::delete_clips_using_selected_media() {
  if (olive::ActiveSequence == nullptr) {
    QMessageBox::critical(this,
                          tr("No active sequence"),
                          tr("No sequence is active, please open the sequence you want to delete clips from."),
                          QMessageBox::Ok);
  } else {
    ComboAction* ca = new ComboAction();
    bool deleted = false;
    QModelIndexList items = get_current_selected();
    QVector<Clip*> sequence_clips = olive::ActiveSequence->GetAllClips();
    for (int i=0;i<sequence_clips.size();i++) {
      Clip* c = sequence_clips.at(i);

      for (int j=0;j<items.size();j++) {
        Media* m = item_to_media(items.at(j));
        if (c->media() == m) {
          ca->append(new DeleteClipAction(c));
          deleted = true;
        }
      }
    }
    for (int j=0;j<items.size();j++) {
      Media* m = item_to_media(items.at(j));
      if (delete_clips_in_clipboard_with_media(ca, m)) deleted = true;
    }
    if (deleted) {
      olive::UndoStack.push(ca);
      update_ui(true);
    } else {
      delete ca;
    }
  }
}

void Project::update_view_type() {
  tree_view->setVisible(olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_TREE);
  icon_view_container->setVisible(olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_ICON
                                  || olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_LIST);


  switch (olive::CurrentConfig.project_view_type) {
  case olive::PROJECT_VIEW_TREE:
    sources_common.view = tree_view;
    break;
  case olive::PROJECT_VIEW_ICON:
  case olive::PROJECT_VIEW_LIST:
    icon_view->setViewMode(olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_ICON ?
                             QListView::IconMode : QListView::ListMode);

    // update list/grid size since they use this value slightly differently
    set_icon_view_size(icon_size_slider->value());

    sources_common.view = icon_view;
    break;
  }
}

void Project::set_icon_view() {
  olive::CurrentConfig.project_view_type = olive::PROJECT_VIEW_ICON;
  update_view_type();
}

void Project::set_list_view()
{
  olive::CurrentConfig.project_view_type = olive::PROJECT_VIEW_LIST;
  update_view_type();
}

void Project::set_tree_view() {
  olive::CurrentConfig.project_view_type = olive::PROJECT_VIEW_TREE;
  update_view_type();
}

void Project::set_icon_view_size(int s) {
  if (icon_view->viewMode() == QListView::IconMode) {
    icon_view->setGridSize(QSize(s, s));
  } else {
    icon_view->setGridSize(QSize());
    icon_view->setIconSize(QSize(s, s));
  }
}

void Project::set_up_dir_enabled() {
  directory_up->setEnabled(icon_view->rootIndex().isValid());
}

void Project::go_up_dir() {
  icon_view->setRootIndex(icon_view->rootIndex().parent());
  set_up_dir_enabled();
}

void Project::make_new_menu() {
  Menu new_menu(this);
  olive::MenuHelper.make_new_menu(&new_menu);
  new_menu.exec(QCursor::pos());
}

QModelIndexList Project::get_current_selected() {
  if (olive::CurrentConfig.project_view_type == olive::PROJECT_VIEW_TREE) {
    return tree_view->selectionModel()->selectedRows();
  }
  return icon_view->selectionModel()->selectedIndexes();
}
