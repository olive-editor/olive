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

#include "sourcescommon.h"

#include "ui/menuhelper.h"
#include "panels/panels.h"
#include "project/media.h"
#include "project/undo.h"
#include "rendering/renderfunctions.h"
#include "panels/timeline.h"
#include "panels/project.h"
#include "project/footage.h"
#include "panels/viewer.h"
#include "project/projectfilter.h"
#include "project/sequence.h"
#include "io/config.h"
#include "dialogs/proxydialog.h"
#include "ui/viewerwidget.h"
#include "io/proxygenerator.h"
#include "mainwindow.h"

#include <QProcess>
#include <QMenu>
#include <QAbstractItemView>
#include <QMimeData>
#include <QMessageBox>
#include <QDesktopServices>

#include <QDebug>

SourcesCommon::SourcesCommon(Project* parent) :
  editing_item(nullptr),
  project_parent(parent)
{
  rename_timer.setInterval(1000);
  connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
}

void SourcesCommon::create_seq_from_selected() {
  if (!selected_items.isEmpty()) {
    QVector<Media*> media_list;
    for (int i=0;i<selected_items.size();i++) {
      media_list.append(project_parent->item_to_media(selected_items.at(i)));
    }

    ComboAction* ca = new ComboAction();
    SequencePtr s = create_sequence_from_media(media_list);

    // add clips to it
    panel_timeline->create_ghosts_from_media(s.get(), 0, media_list);
    panel_timeline->add_clips_from_ghosts(ca, s.get());

    project_parent->create_sequence_internal(ca, s, true, nullptr);
    olive::UndoStack.push(ca);
  }
}

void SourcesCommon::show_context_menu(QWidget* parent, const QModelIndexList& items) {
  QMenu menu(parent);

  selected_items = items;

  QAction* import_action = menu.addAction(tr("Import..."));
  QObject::connect(import_action, SIGNAL(triggered(bool)), project_parent, SLOT(import_dialog()));

  QMenu* new_menu = menu.addMenu(tr("New"));
  olive::MenuHelper.make_new_menu(new_menu);

  QMenu* view_menu = menu.addMenu(tr("View"));

  QAction* tree_view_action = view_menu->addAction(tr("Tree View"));
  connect(tree_view_action, SIGNAL(triggered(bool)), project_parent, SLOT(set_tree_view()));

  QAction* icon_view_action = view_menu->addAction(tr("Icon View"));
  connect(icon_view_action, SIGNAL(triggered(bool)), project_parent, SLOT(set_icon_view()));

  QAction* toolbar_action = view_menu->addAction(tr("Show Toolbar"));
  toolbar_action->setCheckable(true);
  toolbar_action->setChecked(project_parent->toolbar_widget->isVisible());
  connect(toolbar_action, SIGNAL(triggered(bool)), project_parent->toolbar_widget, SLOT(setVisible(bool)));

  QAction* show_sequences = view_menu->addAction(tr("Show Sequences"));
  show_sequences->setCheckable(true);
  show_sequences->setChecked(panel_project->sorter->get_show_sequences());
  connect(show_sequences, SIGNAL(triggered(bool)), panel_project->sorter, SLOT(set_show_sequences(bool)));

  if (items.size() > 0) {
    if (items.size() == 1) {
      Media* first_media = project_parent->item_to_media(items.at(0));

      // replace footage
      int type = first_media->get_type();
      if (type == MEDIA_TYPE_FOOTAGE) {
        QAction* replace_action = menu.addAction(tr("Replace/Relink Media"));
        QObject::connect(replace_action, SIGNAL(triggered(bool)), project_parent, SLOT(replace_selected_file()));

#if defined(Q_OS_WIN)
        QAction* reveal_in_explorer = menu.addAction(tr("Reveal in Explorer"));
#elif defined(Q_OS_MAC)
        QAction* reveal_in_explorer = menu.addAction(tr("Reveal in Finder"));
#else
        QAction* reveal_in_explorer = menu.addAction(tr("Reveal in File Manager"));
#endif
        QObject::connect(reveal_in_explorer, SIGNAL(triggered(bool)), this, SLOT(reveal_in_browser()));
      }
      if (type != MEDIA_TYPE_FOLDER) {
        QAction* replace_clip_media = menu.addAction(tr("Replace Clips Using This Media"));
        QObject::connect(replace_clip_media, SIGNAL(triggered(bool)), project_parent, SLOT(replace_clip_media()));
      }
    }

    // analyze selected footage types
    bool all_sequences = true;
    bool all_footage = true;

    cached_selected_footage.clear();
    for (int i=0;i<items.size();i++) {
      Media* m = project_parent->item_to_media(items.at(i));
      if (m->get_type() != MEDIA_TYPE_SEQUENCE) {
        all_sequences = false;
      }
      if (m->get_type() == MEDIA_TYPE_FOOTAGE) {
        cached_selected_footage.append(m);
      } else {
        all_footage = false;
      }
    }

    // create sequence from
    QAction* create_seq_from = menu.addAction(tr("Create Sequence With This Media"));
    QObject::connect(create_seq_from, SIGNAL(triggered(bool)), this, SLOT(create_seq_from_selected()));

    // ONLY sequences are selected
    if (all_sequences) {
      // ONLY sequences are selected
      QAction* duplicate_action = menu.addAction(tr("Duplicate"));
      QObject::connect(duplicate_action, SIGNAL(triggered(bool)), project_parent, SLOT(duplicate_selected()));
    }

    // ONLY footage is selected
    if (all_footage) {
      QAction* delete_footage_from_sequences = menu.addAction(tr("Delete All Clips Using This Media"));
      QObject::connect(delete_footage_from_sequences, SIGNAL(triggered(bool)), project_parent, SLOT(delete_clips_using_selected_media()));

      QMenu* proxies = menu.addMenu(tr("Proxy"));

      // special case if one footage item is selected and its proxy is currently being generated
      if (cached_selected_footage.size() == 1
          && cached_selected_footage.at(0)->to_footage()->proxy
          && cached_selected_footage.at(0)->to_footage()->proxy_path.isEmpty()) {
        QAction* action = proxies->addAction(tr("Generating proxy: %1% complete").arg(
                                               olive::proxy_generator.get_proxy_progress(cached_selected_footage.at(0))
                                               )
                                             );
        action->setEnabled(false);
      } else {
        // determine whether any selected footage has or doesn't have proxies
        bool footage_without_proxies_exists = false;
        bool footage_with_proxies_exists = false;

        for (int i=0;i<cached_selected_footage.size();i++) {
          if (cached_selected_footage.at(i)->to_footage()->proxy) {
            footage_with_proxies_exists = true;
          } else {
            footage_without_proxies_exists = true;
          }
        }

        // if footage was selected WITHOUT proxies
        if (footage_without_proxies_exists) {
          QString create_proxy_text;

          if (footage_with_proxies_exists) {
            // some of the footage already has proxies, so we use a different string
            create_proxy_text = tr("Create/Modify Proxy");
          } else {
            // none of the footage has proxies
            create_proxy_text = tr("Create Proxy");
          }

          proxies->addAction(create_proxy_text, this, SLOT(open_create_proxy_dialog()));
        }

        // if footage was selected WITH proxies
        if (footage_with_proxies_exists) {

          if (!footage_without_proxies_exists) {
            // if all the footage has proxies, we didn't make a "Create/Modify" above, so we create one here (but only "modify")
            proxies->addAction(tr("Modify Proxy"), this, SLOT(open_create_proxy_dialog()));
          }

          proxies->addAction(tr("Restore Original"), this, SLOT(clear_proxies_from_selected()));
        }
      }
    }

    // delete media
    QAction* delete_action = menu.addAction(tr("Delete"));
    QObject::connect(delete_action, SIGNAL(triggered(bool)), project_parent, SLOT(delete_selected_media()));

    if (items.size() == 1) {
      Media* media_item = project_parent->item_to_media(items.at(0));

      if (media_item->get_type() != MEDIA_TYPE_FOLDER) {
        QAction* preview_in_media_viewer_action = menu.addAction(tr("Preview in Media Viewer"),
                                                                 this,
                                                                 SLOT(OpenSelectedMediaInMediaViewerFromAction()));
        preview_in_media_viewer_action->setData(reinterpret_cast<quintptr>(media_item));
      }

      QAction* properties_action = menu.addAction(tr("Properties..."));
      QObject::connect(properties_action, SIGNAL(triggered(bool)), project_parent, SLOT(open_properties()));
    }
  }

  menu.exec(QCursor::pos());
}

void SourcesCommon::mousePressEvent(QMouseEvent *) {
  stop_rename_timer();
}

void SourcesCommon::item_click(Media *m, const QModelIndex& index) {
  if (editing_item == m) {
    rename_timer.start();
  } else {
    editing_item = m;
    editing_index = index;
  }
}

void SourcesCommon::mouseDoubleClickEvent(const QModelIndexList& selected_items) {
  stop_rename_timer();
  if (selected_items.size() == 0) {
    project_parent->import_dialog();
  } else if (selected_items.size() == 1) {
    Media* media = project_parent->item_to_media(selected_items.at(0));
    if (media->get_type() == MEDIA_TYPE_SEQUENCE) {
      olive::UndoStack.push(new ChangeSequenceAction(media->to_sequence()));
    } else {
      OpenSelectedMediaInMediaViewer(project_parent->item_to_media(selected_items.at(0)));
    }
  }
}

void SourcesCommon::dropEvent(QWidget* parent,
                              QDropEvent *event,
                              const QModelIndex& drop_item,
                              const QModelIndexList& items) {
  const QMimeData* mimeData = event->mimeData();
  MediaPtr m = project_parent->item_to_media_ptr(drop_item);
  if (mimeData->hasUrls()) {
    // drag files in from outside
    QList<QUrl> urls = mimeData->urls();
    if (!urls.isEmpty()) {
      QStringList paths;
      for (int i=0;i<urls.size();i++) {
        paths.append(urls.at(i).toLocalFile());
      }
      bool replace = false;
      if (urls.size() == 1
          && drop_item.isValid()
          && m->get_type() == MEDIA_TYPE_FOOTAGE
          && !QFileInfo(paths.at(0)).isDir()
          && olive::CurrentConfig.drop_on_media_to_replace
          && QMessageBox::question(
            parent,
            tr("Replace Media"),
            tr("You dropped a file onto '%1'. Would you like to replace it with the dropped file?").arg(m->get_name()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        replace = true;
        project_parent->replace_media(m, paths.at(0));
      }
      if (!replace) {
        QModelIndex parent;
        if (drop_item.isValid()) {
          if (m->get_type() == MEDIA_TYPE_FOLDER) {
            parent = drop_item;
          } else {
            parent = drop_item.parent();
          }
        }
        project_parent->process_file_list(paths, false, nullptr, panel_project->item_to_media(parent));
      }
    }
    event->acceptProposedAction();
  } else {
    event->ignore();

    // dragging files within project
    // if we dragged to the root OR dragged to a folder
    if (!drop_item.isValid() || m->get_type() == MEDIA_TYPE_FOLDER) {
      QVector<MediaPtr> move_items;
      for (int i=0;i<items.size();i++) {
        const QModelIndex& item = items.at(i);
        const QModelIndex& parent = item.parent();
        MediaPtr s = project_parent->item_to_media_ptr(item);
        if (parent != drop_item && item != drop_item) {
          bool ignore = false;
          if (parent.isValid()) {
            // if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
            QModelIndex par = parent;
            while (par.isValid() && !ignore) {
              for (int j=0;j<items.size();j++) {
                if (par == items.at(j)) {
                  ignore = true;
                  break;
                }
              }
              par = par.parent();
            }
          }
          if (!ignore) {
            move_items.append(s);
          }
        }
      }
      if (move_items.size() > 0) {
        MediaMove* mm = new MediaMove();
        mm->to = m.get();
        mm->items = move_items;
        olive::UndoStack.push(mm);
      }
    }
  }
}

void SourcesCommon::reveal_in_browser() {
  Media* media = project_parent->item_to_media(selected_items.at(0));
  Footage* m = media->to_footage();

#if defined(Q_OS_WIN)
  QStringList args;
  args << "/select," << QDir::toNativeSeparators(m->url);
  QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MAC)
  QStringList args;
  args << "-e";
  args << "tell application \"Finder\"";
  args << "-e";
  args << "activate";
  args << "-e";
  args << "select POSIX file \""+m->url+"\"";
  args << "-e";
  args << "end tell";
  QProcess::startDetached("osascript", args);
#else
  QDesktopServices::openUrl(QUrl::fromLocalFile(m->url.left(m->url.lastIndexOf('/'))));
#endif
}

void SourcesCommon::stop_rename_timer() {
  rename_timer.stop();
}

void SourcesCommon::rename_interval() {
  stop_rename_timer();
  if (view->hasFocus() && editing_item != nullptr) {
    view->edit(editing_index);
  }
}

void SourcesCommon::item_renamed(Media* item) {
  if (editing_item == item) {
    MediaRename* mr = new MediaRename(item, "idk");
    olive::UndoStack.push(mr);
    editing_item = nullptr;
  }
}

void SourcesCommon::OpenSelectedMediaInMediaViewerFromAction()
{
  OpenSelectedMediaInMediaViewer(reinterpret_cast<Media*>(static_cast<QAction*>(sender())->data().value<quintptr>()));
}

void SourcesCommon::OpenSelectedMediaInMediaViewer(Media* item) {
  if (item->get_type() != MEDIA_TYPE_FOLDER) {
    panel_footage_viewer->set_media(item);
    panel_footage_viewer->setFocus();
  }
}

void SourcesCommon::open_create_proxy_dialog() {
  // open the proxy dialog and send it a list of currently selected footage
  ProxyDialog pd(olive::MainWindow, cached_selected_footage);
  pd.exec();
}

void SourcesCommon::clear_proxies_from_selected() {
  QList<QString> delete_list;

  for (int i=0;i<cached_selected_footage.size();i++) {
    Footage* f = cached_selected_footage.at(i)->to_footage();

    if (f->proxy && !f->proxy_path.isEmpty()) {
      if (QFileInfo::exists(f->proxy_path)) {
        if (QMessageBox::question(olive::MainWindow,
                                  tr("Delete proxy"),
                                  tr("Would you like to delete the proxy file \"%1\" as well?").arg(f->proxy_path),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
          delete_list.append(f->proxy_path);
        }
      }
    }

    f->proxy = false;
    f->proxy_path.clear();
  }

  if (olive::ActiveSequence != nullptr) {
    // close all clips so we can delete any proxies requested to be deleted
    close_active_clips(olive::ActiveSequence.get());
  }

  // delete proxies requested to be deleted
  for (int i=0;i<delete_list.size();i++) {
    QFile::remove(delete_list.at(i));
  }

  if (olive::ActiveSequence != nullptr) {
    // update viewer (will re-open active clips with original media)
    panel_sequence_viewer->viewer_widget->frame_update();
  }

  olive::MainWindow->setWindowModified(true);
}
