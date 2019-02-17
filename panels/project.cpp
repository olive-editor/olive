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

#include "oliveglobal.h"

#include "panels.h"
#include "playback/playback.h"
#include "io/previewgenerator.h"
#include "project/undo.h"
#include "mainwindow.h"
#include "io/config.h"
#include "playback/cacher.h"
#include "dialogs/replaceclipmediadialog.h"
#include "panels/effectcontrols.h"
#include "dialogs/newsequencedialog.h"
#include "dialogs/mediapropertiesdialog.h"
#include "dialogs/loaddialog.h"
#include "io/clipboard.h"
#include "ui/sourcetable.h"
#include "ui/sourceiconview.h"
#include "ui/menuhelper.h"
#include "ui/mediaiconservice.h"
#include "project/sourcescommon.h"
#include "project/projectfilter.h"
#include "debug.h"

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
#include <QMenu>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#define MAXIMUM_RECENT_PROJECTS 10 // FIXME: should be configurable

QString autorecovery_filename;
QStringList recent_projects;

Project::Project(QWidget *parent) :
  Panel(parent)
{
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QWidget* dockWidgetContents = new QWidget(this);

  QVBoxLayout* verticalLayout = new QVBoxLayout(dockWidgetContents);
  verticalLayout->setMargin(0);
  verticalLayout->setSpacing(0);

  setWidget(dockWidgetContents);

  sources_common = new SourcesCommon(this);

  sorter = new ProjectFilter(this);
  sorter->setSourceModel(&olive::project_model);

  // optional toolbar
  toolbar_widget = new QWidget();
  toolbar_widget->setVisible(olive::CurrentConfig.show_project_toolbar);
  toolbar_widget->setObjectName("project_toolbar");

  QHBoxLayout* toolbar = new QHBoxLayout(toolbar_widget);
  toolbar->setMargin(0);
  toolbar->setSpacing(0);

  QPushButton* toolbar_new = new QPushButton();
  QIcon icon1;
  icon1.addFile(QStringLiteral(":/icons/add-button.png"), QSize(), QIcon::Normal, QIcon::On);
  icon1.addFile(QStringLiteral(":/icons/add-button-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_new->setIcon(icon1);
  toolbar_new->setToolTip("New");
  connect(toolbar_new, SIGNAL(clicked(bool)), this, SLOT(make_new_menu()));
  toolbar->addWidget(toolbar_new);

  QPushButton* toolbar_open = new QPushButton();
  QIcon icon2;
  icon2.addFile(QStringLiteral(":/icons/open.png"), QSize(), QIcon::Normal, QIcon::On);
  icon2.addFile(QStringLiteral(":/icons/open-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_open->setIcon(icon2);
  toolbar_open->setToolTip("Open Project");
  connect(toolbar_open, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(open_project()));
  toolbar->addWidget(toolbar_open);

  QPushButton* toolbar_save = new QPushButton();
  QIcon icon3;
  icon3.addFile(QStringLiteral(":/icons/save.png"), QSize(), QIcon::Normal, QIcon::On);
  icon3.addFile(QStringLiteral(":/icons/save-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_save->setIcon(icon3);
  toolbar_save->setToolTip("Save Project");
  connect(toolbar_save, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(save_project()));
  toolbar->addWidget(toolbar_save);

  QPushButton* toolbar_undo = new QPushButton();
  QIcon icon4;
  icon4.addFile(QStringLiteral(":/icons/undo.png"), QSize(), QIcon::Normal, QIcon::On);
  icon4.addFile(QStringLiteral(":/icons/undo-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_undo->setIcon(icon4);
  toolbar_undo->setToolTip("Undo");
  connect(toolbar_undo, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(undo()));
  toolbar->addWidget(toolbar_undo);

  QPushButton* toolbar_redo = new QPushButton();
  QIcon icon5;
  icon5.addFile(QStringLiteral(":/icons/redo.png"), QSize(), QIcon::Normal, QIcon::On);
  icon5.addFile(QStringLiteral(":/icons/redo-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_redo->setIcon(icon5);
  toolbar_redo->setToolTip("Redo");
  connect(toolbar_redo, SIGNAL(clicked(bool)), olive::Global.get(), SLOT(redo()));
  toolbar->addWidget(toolbar_redo);

  toolbar_search = new QLineEdit();
  toolbar_search->setClearButtonEnabled(true);
  connect(toolbar_search, SIGNAL(textChanged(QString)), sorter, SLOT(update_search_filter(const QString&)));
  toolbar->addWidget(toolbar_search);

  QPushButton* toolbar_tree_view = new QPushButton();
  QIcon icon6;
  icon6.addFile(QStringLiteral(":/icons/treeview.png"), QSize(), QIcon::Normal, QIcon::On);
  icon6.addFile(QStringLiteral(":/icons/treeview-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_tree_view->setIcon(icon6);
  toolbar_tree_view->setToolTip("Tree View");
  connect(toolbar_tree_view, SIGNAL(clicked(bool)), this, SLOT(set_tree_view()));
  toolbar->addWidget(toolbar_tree_view);

  QPushButton* toolbar_icon_view = new QPushButton();
  QIcon icon7;
  icon7.addFile(QStringLiteral(":/icons/iconview.png"), QSize(), QIcon::Normal, QIcon::On);
  icon7.addFile(QStringLiteral(":/icons/iconview-disabled.png"), QSize(), QIcon::Disabled, QIcon::On);
  toolbar_icon_view->setIcon(icon7);
  toolbar_icon_view->setToolTip("Icon View");
  connect(toolbar_icon_view, SIGNAL(clicked(bool)), this, SLOT(set_icon_view()));
  toolbar->addWidget(toolbar_icon_view);

  verticalLayout->addWidget(toolbar_widget);

  // tree view
  tree_view = new SourceTable();
  tree_view->project_parent = this;
  tree_view->setModel(sorter);
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

  QIcon directory_up_button;
  directory_up_button.addFile(":/icons/dirup.png", QSize(), QIcon::Normal);
  directory_up_button.addFile(":/icons/dirup-disabled.png", QSize(), QIcon::Disabled);

  directory_up = new QPushButton();
  directory_up->setIcon(directory_up_button);
  directory_up->setEnabled(false);
  icon_view_controls->addWidget(directory_up);

  icon_view_controls->addStretch();

  QSlider* icon_size_slider = new QSlider(Qt::Horizontal);
  icon_size_slider->setMinimum(16);
  icon_size_slider->setMaximum(120);
  icon_view_controls->addWidget(icon_size_slider);
  connect(icon_size_slider, SIGNAL(valueChanged(int)), this, SLOT(set_icon_view_size(int)));

  icon_view_container_layout->addLayout(icon_view_controls);

  icon_view = new SourceIconView();
  icon_view->project_parent = this;
  icon_view->setModel(sorter);
  icon_view->setIconSize(QSize(100, 100));
  icon_view->setViewMode(QListView::IconMode);
  icon_view->setUniformItemSizes(true);
  icon_view_container_layout->addWidget(icon_view);

  icon_size_slider->setValue(icon_view->iconSize().height());

  verticalLayout->addWidget(icon_view_container);

  connect(directory_up, SIGNAL(clicked(bool)), this, SLOT(go_up_dir()));
  connect(icon_view, SIGNAL(changed_root()), this, SLOT(set_up_dir_enabled()));

  update_view_type();

  Retranslate();
}

Project::~Project() {
  delete sorter;
}

void Project::Retranslate() {
  toolbar_search->setPlaceholderText(tr("Search media, markers, etc."));
  setWindowTitle(tr("Project"));
}

QString Project::get_next_sequence_name(QString start) {
  if (start.isEmpty()) start = tr("Sequence");

  int n = 1;
  bool found = true;
  QString name;
  while (found) {
    found = false;
    name = start + " ";
    if (n < 10) {
      name += "0";
    }
    name += QString::number(n);
    for (int i=0;i<olive::project_model.childCount();i++) {
      if (QString::compare(olive::project_model.child(i)->get_name(), name, Qt::CaseInsensitive) == 0) {
        found = true;
        n++;
        break;
      }
    }
  }
  return name;
}

SequencePtr create_sequence_from_media(QVector<Media*>& media_list) {
  SequencePtr s(new Sequence());

  s->name = panel_project->get_next_sequence_name();

  // shitty hardcoded default values
  s->width = 1920;
  s->height = 1080;
  s->frame_rate = 29.97;
  s->audio_frequency = 48000;
  s->audio_layout = 3;

  bool got_video_values = false;
  bool got_audio_values = false;
  for (int i=0;i<media_list.size();i++) {
    Media* media = media_list.at(i);
    switch (media->get_type()) {
    case MEDIA_TYPE_FOOTAGE:
    {
      FootagePtr m = media->to_footage();
      if (m->ready) {
        if (!got_video_values) {
          for (int j=0;j<m->video_tracks.size();j++) {
            const FootageStream& ms = m->video_tracks.at(j);
            s->width = ms.video_width;
            s->height = ms.video_height;
            if (!qFuzzyCompare(ms.video_frame_rate, 0.0)) {
              s->frame_rate = ms.video_frame_rate * m->speed;

              if (ms.video_interlacing != VIDEO_PROGRESSIVE) s->frame_rate *= 2;

              // only break with a decent frame rate, otherwise there may be a better candidate
              got_video_values = true;
              break;
            }
          }
        }
        if (!got_audio_values && m->audio_tracks.size() > 0) {
          const FootageStream& ms = m->audio_tracks.at(0);
          s->audio_frequency = ms.audio_frequency;
          got_audio_values = true;
        }
      }
    }
      break;
    case MEDIA_TYPE_SEQUENCE:
    {
      SequencePtr seq = media->to_sequence();
      s->width = seq->width;
      s->height = seq->height;
      s->frame_rate = seq->frame_rate;
      s->audio_frequency = seq->audio_frequency;
      s->audio_layout = seq->audio_layout;

      got_video_values = true;
      got_audio_values = true;
    }
      break;
    }
    if (got_video_values && got_audio_values) break;
  }

  return s;
}

void Project::duplicate_selected() {
  QModelIndexList items = get_current_selected();
  bool duped = false;
  ComboAction* ca = new ComboAction();
  for (int j=0;j<items.size();j++) {
    Media* i = item_to_media(items.at(j));
    if (i->get_type() == MEDIA_TYPE_SEQUENCE) {
      create_sequence_internal(ca, i->to_sequence()->copy(), false, item_to_media(items.at(j).parent()));
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
    Media* item = item_to_media(selected_items.at(0));
    if (item->get_type() == MEDIA_TYPE_FOOTAGE) {
      replace_media(item, nullptr);
    }
  }
}

void Project::replace_media(Media* item, QString filename) {
  if (filename.isEmpty()) {
    filename = QFileDialog::getOpenFileName(
          this,
          tr("Replace '%1'").arg(item->get_name()),
          "",
          tr("All Files") + " (*)");
  }
  if (!filename.isEmpty()) {
    ReplaceMediaCommand* rmc = new ReplaceMediaCommand(item, filename);
    olive::UndoStack.push(rmc);
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
  Media* m = create_folder_internal(nullptr);
  olive::UndoStack.push(new AddMediaCommand(m, get_selected_folder()));

  QModelIndex index = olive::project_model.create_index(m->row(), 0, m);
  switch (olive::CurrentConfig.project_view_type) {
  case PROJECT_VIEW_TREE:
    tree_view->edit(sorter->mapFromSource(index));
    break;
  case PROJECT_VIEW_ICON:
    icon_view->edit(sorter->mapFromSource(index));
    break;
  }
}

void Project::new_sequence() {
  NewSequenceDialog nsd(this);
  nsd.set_sequence_name(get_next_sequence_name());
  nsd.exec();
}

Media* Project::create_sequence_internal(ComboAction *ca, SequencePtr s, bool open, Media* parent) {
  if (parent == nullptr) {
    parent = olive::project_model.get_root();
  }

  Media* item(new Media(parent));
  item->set_sequence(s);

  if (ca != nullptr) {
    ca->append(new NewSequenceCommand(item, parent));
    if (open) ca->append(new ChangeSequenceAction(s));
  } else {
    if (parent == olive::project_model.get_root()) {
      olive::project_model.appendChild(parent, item);
    } else {
      parent->appendChild(item);
    }
    if (open) set_sequence(s);
  }
  return item;
}

QString Project::get_file_name_from_path(const QString& path) {
  return path.mid(path.lastIndexOf('/')+1);
}

/*Media* Project::new_item() {
    Media* item = new Media(0);
  //item->setFlags(item->flags() | Qt::ItemIsEditable);
  return item;
}*/

bool Project::is_focused() {
  return tree_view->hasFocus() || icon_view->hasFocus();
}

Media* Project::create_folder_internal(QString name) {
  Media* item = new Media(nullptr);
  item->set_folder();
  item->set_name(name);
  return item;
}

Media *Project::item_to_media(const QModelIndex &index) {
  return static_cast<Media*>(sorter->mapToSource(index).internalPointer());
  //    return static_cast<Media*>(index.internalPointer());
}

void Project::get_all_media_from_table(QList<Media*>& items, QList<Media*>& list, int search_type) {
  for (int i=0;i<items.size();i++) {
    Media* item = items.at(i);
    if (item->get_type() == MEDIA_TYPE_FOLDER) {
      QList<Media*> children;
      for (int j=0;j<item->childCount();j++) {
        children.append(item->child(j));
      }
      get_all_media_from_table(children, list, search_type);
    } else if (search_type == item->get_type() || search_type == -1) {
      list.append(item);
    }
  }
}

bool delete_clips_in_clipboard_with_media(ComboAction* ca, Media* m) {
  int delete_count = 0;
  if (clipboard_type == CLIPBOARD_TYPE_CLIP) {
    for (int i=0;i<clipboard.size();i++) {
      ClipPtr c = std::static_pointer_cast<Clip>(clipboard.at(i));
      if (c->media == m) {
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
  QList<Media*> sequence_items;
  QList<Media*> all_top_level_items;
  for (int i=0;i<olive::project_model.childCount();i++) {
    all_top_level_items.append(olive::project_model.child(i));
  }
  get_all_media_from_table(all_top_level_items, sequence_items, MEDIA_TYPE_SEQUENCE); // find all sequences in project
  if (sequence_items.size() > 0) {
    QList<Media*> media_items;
    get_all_media_from_table(items, media_items, MEDIA_TYPE_FOOTAGE);
    for (int i=0;i<media_items.size();i++) {
      Media* item = media_items.at(i);
      FootagePtr media = item->to_footage();
      bool confirm_delete = false;
      for (int j=0;j<sequence_items.size();j++) {
        SequencePtr s = sequence_items.at(j)->to_sequence();
        for (int k=0;k<s->clips.size();k++) {
          ClipPtr c = s->clips.at(k);
          if (c != nullptr && c->media == item) {
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

                j = sequence_items.size();
                k = s->clips.size();
              } else if (confirm.clickedButton() == abort_button) {
                // break out of loop
                i = media_items.size();
                j = sequence_items.size();
                k = s->clips.size();

                remove = false;
              }
            }
            if (confirm_delete) {
              ca->append(new DeleteClipAction(s, k));
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
    panel_effect_controls->clear_effects(true);
    if (olive::ActiveSequence != nullptr) olive::ActiveSequence->selections.clear();

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
      ca->append(new DeleteMediaCommand(items.at(i)));

      if (items.at(i)->get_type() == MEDIA_TYPE_SEQUENCE) {
        redraw = true;

        SequencePtr s = items.at(i)->to_sequence();

        if (s == olive::ActiveSequence) {
          ca->append(new ChangeSequenceAction(nullptr));
        }

        if (s == panel_footage_viewer->seq) {
          panel_footage_viewer->set_media(nullptr);
        }
      } else if (items.at(i)->get_type() == MEDIA_TYPE_FOOTAGE) {
        if (panel_footage_viewer->seq != nullptr) {
          for (int j=0;j<panel_footage_viewer->seq->clips.size();j++) {
            ClipPtr c = panel_footage_viewer->seq->clips.at(j);
            if (c != nullptr && c->media == items.at(i)) {
              panel_footage_viewer->set_media(nullptr);
              break;
            }
          }
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

void Project::start_preview_generator(Media* item, bool replacing) {
  // set up throbber animation
  olive::media_icon_service->SetMediaIcon(item, ICON_TYPE_LOADING);

  PreviewGenerator* pg = new PreviewGenerator(item, item->to_footage(), replacing);
  item->to_footage()->preview_gen = pg;
  pg->start(QThread::LowPriority);
}

void Project::process_file_list(QStringList& files, bool recursive, Media* replace, Media* parent) {
  bool imported = false;

  QVector<QString> image_sequence_urls;
  QVector<bool> image_sequence_importassequence;
  QStringList image_sequence_formats = olive::CurrentConfig.img_seq_formats.split("|");

  if (!recursive) last_imported_media.clear();

  bool create_undo_action = (!recursive && replace == nullptr);
  ComboAction* ca = nullptr;
  if (create_undo_action) ca = new ComboAction();

  for (int i=0;i<files.size();i++) {
    if (QFileInfo(files.at(i)).isDir()) {
      QString folder_name = get_file_name_from_path(files.at(i));
      Media* folder = create_folder_internal(folder_name);

      QDir directory(files.at(i));
      directory.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

      QFileInfoList subdir_files = directory.entryInfoList();
      QStringList subdir_filenames;

      for (int j=0;j<subdir_files.size();j++) {
        subdir_filenames.append(subdir_files.at(j).filePath());
      }

      process_file_list(subdir_filenames, true, nullptr, folder);

      if (create_undo_action) {
        ca->append(new AddMediaCommand(folder, parent));
      } else {
        olive::project_model.appendChild(parent, folder);
      }

      imported = true;
    } else if (!files.at(i).isEmpty()) {
      QString file(files.at(i));
      bool skip = false;

      /* Heuristic to determine whether file is part of an image sequence */

      // check file extension (assume it's not a

      int lastcharindex = file.lastIndexOf(".");
      bool found = true;
      if (lastcharindex != -1 && lastcharindex > file.lastIndexOf('/')) {
        // image_sequence_formats
        found = false;
        QString ext = file.mid(lastcharindex+1);
        for (int j=0;j<image_sequence_formats.size();j++) {
          if (ext == image_sequence_formats.at(j)) {
            found = true;
            break;
          }
        }
      } else {
        lastcharindex = file.length();
      }

      if (lastcharindex == 0) lastcharindex++;

      if (found && file[lastcharindex-1].isDigit()) {
        bool is_img_sequence = false;

        // how many digits are in the filename?
        int digit_count = 0;
        int digit_test = lastcharindex-1;
        while (file[digit_test].isDigit()) {
          digit_count++;
          digit_test--;
        }

        // retrieve number from filename
        digit_test++;
        int file_number = file.mid(digit_test, digit_count).toInt();

        // Check if there are files with the same filename but just different numbers
        if (QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number-1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))
            || QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number+1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))) {
          is_img_sequence = true;
        }

        if (is_img_sequence) {
          // get the URL that we would pass to FFmpeg to force it to read the image as a sequence
          QString new_filename = file.left(digit_test) + "%" + QString::number(digit_count) + "d" + file.mid(lastcharindex);

          // add image sequence url to a vector in case the user imported several files that
          // we're interpreting as a possible sequence
          found = false;
          for (int i=0;i<image_sequence_urls.size();i++) {
            if (image_sequence_urls.at(i) == new_filename) {
              // either SKIP if we're importing as a sequence, or leave it if we aren't
              if (image_sequence_importassequence.at(i)) {
                skip = true;
              }
              found = true;
              break;
            }
          }
          if (!found) {
            image_sequence_urls.append(new_filename);
            if (QMessageBox::question(this,
                                      tr("Image sequence detected"),
                                      tr("The file '%1' appears to be part of an image sequence. Would you like to import it as such?").arg(file),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes) == QMessageBox::Yes) {
              file = new_filename;
              image_sequence_importassequence.append(true);
            } else {
              image_sequence_importassequence.append(false);
            }
          }
        }
      }

      if (!skip) {
        Media* item;
        FootagePtr m;

        if (replace != nullptr) {
          item = replace;
          m = replace->to_footage();
          m->reset();
        } else {
          item = new Media(parent);
          m = FootagePtr(new Footage());
        }

        m->using_inout = false;
        m->url = file;
        m->name = get_file_name_from_path(files.at(i));

        item->set_footage(m);

        last_imported_media.append(item);

        if (replace == nullptr) {
          if (create_undo_action) {
            ca->append(new AddMediaCommand(item, parent));
          } else {
            parent->appendChild(item);
            //						project_model.appendChild(parent, item);
          }
        }

        imported = true;
      }
    }
  }
  if (create_undo_action) {
    if (imported) {
      olive::UndoStack.push(ca);

      for (int i=0;i<last_imported_media.size();i++) {
        // generate waveform/thumbnail in another thread
        start_preview_generator(last_imported_media.at(i), replace != nullptr);
      }
    } else {
      delete ca;
    }
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
      QModelIndex sorted_index = sorter->mapFromSource(item);

      // retrieve its parent item
      QModelIndex hierarchy = sorted_index.parent();

      if (olive::CurrentConfig.project_view_type == PROJECT_VIEW_TREE) {

        // if we're in tree view, expand every folder in the hierarchy containing the media
        while (hierarchy.isValid()) {
          tree_view->setExpanded(hierarchy, true);
          hierarchy = hierarchy.parent();
        }

        // select item (requires a QItemSelection object to select the whole row)
        QItemSelection row_select(
              sorter->index(sorted_index.row(), 0, sorted_index.parent()),
              sorter->index(sorted_index.row(), sorter->columnCount()-1, sorted_index.parent())
              );

        tree_view->selectionModel()->select(row_select, QItemSelectionModel::Select);
      } else if (olive::CurrentConfig.project_view_type == PROJECT_VIEW_ICON) {

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

void Project::import_dialog() {
  QFileDialog fd(this, tr("Import media..."), "", tr("All Files") + " (*)");
  fd.setFileMode(QFileDialog::ExistingFiles);

  if (fd.exec()) {
    QStringList files = fd.selectedFiles();
    process_file_list(files, false, nullptr, get_selected_folder());
  }
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
    for (int i=0;i<olive::ActiveSequence->clips.size();i++) {
      const ClipPtr& c = olive::ActiveSequence->clips.at(i);
      if (c != nullptr) {
        for (int j=0;j<items.size();j++) {
          Media* m = item_to_media(items.at(j));
          if (c->media == m) {
            ca->append(new DeleteClipAction(olive::ActiveSequence, i));
            deleted = true;
          }
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

void Project::clear() {
  // clear effects cache
  panel_effect_controls->clear_effects(true);

  // delete sequences first because it's important to close all the clips before deleting the media
  QVector<Media*> sequences = list_all_project_sequences();
  for (int i=0;i<sequences.size();i++) {
    sequences.at(i)->to_sequence().reset();
    sequences.at(i)->set_sequence(nullptr);
  }

  // delete everything else
  olive::project_model.clear();

  // update tree view (sometimes this doesn't seem to update reliably)
  tree_view->update();
}

void Project::new_project() {
  // clear existing project
  set_sequence(nullptr);
  panel_footage_viewer->set_media(nullptr);
  clear();
  olive::MainWindow->setWindowModified(false);
}

void Project::load_project(bool autorecovery) {
  new_project();

  LoadDialog ld(this, autorecovery);
  ld.exec();
}

void save_marker(QXmlStreamWriter& stream, const Marker& m) {
  stream.writeStartElement("marker");
  stream.writeAttribute("frame", QString::number(m.frame));
  stream.writeAttribute("name", m.name);
  stream.writeEndElement();
}

void Project::save_folder(QXmlStreamWriter& stream, int type, bool set_ids_only, const QModelIndex& parent) {
  for (int i=0;i<olive::project_model.rowCount(parent);i++) {
    const QModelIndex& item = olive::project_model.index(i, 0, parent);
    Media* m = olive::project_model.getItem(item);

    if (type == m->get_type()) {
      if (m->get_type() == MEDIA_TYPE_FOLDER) {
        if (set_ids_only) {
          m->temp_id = folder_id; // saves a temporary ID for matching in the project file
          folder_id++;
        } else {
          // if we're saving folders, save the folder
          stream.writeStartElement("folder");
          stream.writeAttribute("name", m->get_name());
          stream.writeAttribute("id", QString::number(m->temp_id));
          if (!item.parent().isValid()) {
            stream.writeAttribute("parent", "0");
          } else {
            stream.writeAttribute("parent", QString::number(olive::project_model.getItem(item.parent())->temp_id));
          }
          stream.writeEndElement();
        }
        // save_folder(stream, item, type, set_ids_only);
      } else {
        int folder = m->parentItem()->temp_id;
        if (type == MEDIA_TYPE_FOOTAGE) {
          FootagePtr f = m->to_footage();
          f->save_id = media_id;
          stream.writeStartElement("footage");
          stream.writeAttribute("id", QString::number(media_id));
          stream.writeAttribute("folder", QString::number(folder));
          stream.writeAttribute("name", f->name);
          stream.writeAttribute("url", proj_dir.relativeFilePath(f->url));
          stream.writeAttribute("duration", QString::number(f->length));
          stream.writeAttribute("using_inout", QString::number(f->using_inout));
          stream.writeAttribute("in", QString::number(f->in));
          stream.writeAttribute("out", QString::number(f->out));
          stream.writeAttribute("speed", QString::number(f->speed));
          stream.writeAttribute("alphapremul", QString::number(f->alpha_is_premultiplied));

          stream.writeAttribute("proxy", QString::number(f->proxy));
          stream.writeAttribute("proxypath", f->proxy_path);

          // save video stream metadata
          for (int j=0;j<f->video_tracks.size();j++) {
            const FootageStream& ms = f->video_tracks.at(j);
            stream.writeStartElement("video");
            stream.writeAttribute("id", QString::number(ms.file_index));
            stream.writeAttribute("width", QString::number(ms.video_width));
            stream.writeAttribute("height", QString::number(ms.video_height));
            stream.writeAttribute("framerate", QString::number(ms.video_frame_rate, 'f', 10));
            stream.writeAttribute("infinite", QString::number(ms.infinite_length));
            stream.writeEndElement(); // video
          }

          // save audio stream metadata
          for (int j=0;j<f->audio_tracks.size();j++) {
            const FootageStream& ms = f->audio_tracks.at(j);
            stream.writeStartElement("audio");
            stream.writeAttribute("id", QString::number(ms.file_index));
            stream.writeAttribute("channels", QString::number(ms.audio_channels));
            stream.writeAttribute("layout", QString::number(ms.audio_layout));
            stream.writeAttribute("frequency", QString::number(ms.audio_frequency));
            stream.writeEndElement(); // audio
          }

          // save footage markers
          for (int j=0;j<f->markers.size();j++) {
            save_marker(stream, f->markers.at(j));
          }

          stream.writeEndElement(); // footage
          media_id++;
        } else if (type == MEDIA_TYPE_SEQUENCE) {
          SequencePtr s = m->to_sequence();
          if (set_ids_only) {
            s->save_id = sequence_id;
            sequence_id++;
          } else {
            stream.writeStartElement("sequence");
            stream.writeAttribute("id", QString::number(s->save_id));
            stream.writeAttribute("folder", QString::number(folder));
            stream.writeAttribute("name", s->name);
            stream.writeAttribute("width", QString::number(s->width));
            stream.writeAttribute("height", QString::number(s->height));
            stream.writeAttribute("framerate", QString::number(s->frame_rate, 'f', 10));
            stream.writeAttribute("afreq", QString::number(s->audio_frequency));
            stream.writeAttribute("alayout", QString::number(s->audio_layout));
            if (s == olive::ActiveSequence) {
              stream.writeAttribute("open", "1");
            }
            stream.writeAttribute("workarea", QString::number(s->using_workarea));
            stream.writeAttribute("workareaIn", QString::number(s->workarea_in));
            stream.writeAttribute("workareaOut", QString::number(s->workarea_out));

            /*
            for (int j=0;j<s->transitions.size();j++) {
                            TransitionPtr t = s->transitions.at(j);
              if (t != nullptr) {
                stream.writeStartElement("transition");
                stream.writeAttribute("id", QString::number(j));
                stream.writeAttribute("length", QString::number(t->get_true_length()));
                t->save(stream);
                stream.writeEndElement(); // transition
              }
            }
                        */

            for (int j=0;j<s->clips.size();j++) {
              const ClipPtr& c = s->clips.at(j);
              if (c != nullptr) {
                stream.writeStartElement("clip"); // clip
                stream.writeAttribute("id", QString::number(j));
                stream.writeAttribute("enabled", QString::number(c->enabled));
                stream.writeAttribute("name", c->name);
                stream.writeAttribute("clipin", QString::number(c->clip_in));
                stream.writeAttribute("in", QString::number(c->timeline_in));
                stream.writeAttribute("out", QString::number(c->timeline_out));
                stream.writeAttribute("track", QString::number(c->track));
                /*
                stream.writeAttribute("opening", QString::number(c->opening_transition));
                stream.writeAttribute("closing", QString::number(c->closing_transition));
                */

                stream.writeAttribute("r", QString::number(c->color_r));
                stream.writeAttribute("g", QString::number(c->color_g));
                stream.writeAttribute("b", QString::number(c->color_b));

                stream.writeAttribute("autoscale", QString::number(c->autoscale));
                stream.writeAttribute("speed", QString::number(c->speed, 'f', 10));
                stream.writeAttribute("maintainpitch", QString::number(c->maintain_audio_pitch));
                stream.writeAttribute("reverse", QString::number(c->reverse));

                if (c->media != nullptr) {
                  stream.writeAttribute("type", QString::number(c->media->get_type()));
                  switch (c->media->get_type()) {
                  case MEDIA_TYPE_FOOTAGE:
                    stream.writeAttribute("media", QString::number(c->media->to_footage()->save_id));
                    stream.writeAttribute("stream", QString::number(c->media_stream));
                    break;
                  case MEDIA_TYPE_SEQUENCE:
                    stream.writeAttribute("sequence", QString::number(c->media->to_sequence()->save_id));
                    break;
                  }
                }

                // save markers
                // only necessary for null media clips, since media has its own markers
                if (c->media == nullptr) {
                  for (int k=0;k<c->get_markers().size();k++) {
                    save_marker(stream, c->get_markers().at(k));
                  }
                }

                // save clip links
                stream.writeStartElement("linked"); // linked
                for (int k=0;k<c->linked.size();k++) {
                  stream.writeStartElement("link"); // link
                  stream.writeAttribute("id", QString::number(c->linked.at(k)));
                  stream.writeEndElement(); // link
                }
                stream.writeEndElement(); // linked

                for (int k=0;k<c->effects.size();k++) {
                  stream.writeStartElement("effect"); // effect
                  c->effects.at(k)->save(stream);
                  stream.writeEndElement(); // effect
                }

                stream.writeEndElement(); // clip
              }
            }
            for (int j=0;j<s->markers.size();j++) {
              save_marker(stream, s->markers.at(j));
            }
            stream.writeEndElement();
          }
        }
      }
    }

    if (m->get_type() == MEDIA_TYPE_FOLDER) {
      save_folder(stream, type, set_ids_only, item);
    }
  }
}

void Project::save_project(bool autorecovery) {
  folder_id = 1;
  media_id = 1;
  sequence_id = 1;

  QFile file(autorecovery ? autorecovery_filename : olive::ActiveProjectFilename);
  if (!file.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
    qCritical() << "Could not open file";
    return;
  }

  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  stream.writeStartDocument(); // doc

  stream.writeStartElement("project"); // project

  stream.writeTextElement("version", QString::number(SAVE_VERSION));

  stream.writeTextElement("url", olive::ActiveProjectFilename);
  proj_dir = QFileInfo(olive::ActiveProjectFilename).absoluteDir();

  save_folder(stream, MEDIA_TYPE_FOLDER, true);

  stream.writeStartElement("folders"); // folders
  save_folder(stream, MEDIA_TYPE_FOLDER, false);
  stream.writeEndElement(); // folders

  stream.writeStartElement("media"); // media
  save_folder(stream, MEDIA_TYPE_FOOTAGE, false);
  stream.writeEndElement(); // media

  save_folder(stream, MEDIA_TYPE_SEQUENCE, true);

  stream.writeStartElement("sequences"); // sequences
  save_folder(stream, MEDIA_TYPE_SEQUENCE, false);
  stream.writeEndElement();// sequences

  stream.writeEndElement(); // project

  stream.writeEndDocument(); // doc

  file.close();

  if (!autorecovery) {
    add_recent_project(olive::ActiveProjectFilename);
    olive::MainWindow->setWindowModified(false);
  }
}

void Project::update_view_type() {
  tree_view->setVisible(olive::CurrentConfig.project_view_type == PROJECT_VIEW_TREE);
  icon_view_container->setVisible(olive::CurrentConfig.project_view_type == PROJECT_VIEW_ICON);

  switch (olive::CurrentConfig.project_view_type) {
  case PROJECT_VIEW_TREE:
    sources_common->view = tree_view;
    break;
  case PROJECT_VIEW_ICON:
    sources_common->view = icon_view;
    break;
  }
}

void Project::set_icon_view() {
  olive::CurrentConfig.project_view_type = PROJECT_VIEW_ICON;
  update_view_type();
}

void Project::set_tree_view() {
  olive::CurrentConfig.project_view_type = PROJECT_VIEW_TREE;
  update_view_type();
}

void Project::save_recent_projects() {
  // save to file
  QFile f(olive::Global->get_recent_project_list_file());
  if (f.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
    QTextStream out(&f);
    for (int i=0;i<recent_projects.size();i++) {
      if (i > 0) {
        out << "\n";
      }
      out << recent_projects.at(i);
    }
    f.close();
  } else {
    qWarning() << "Could not save recent projects";
  }
}

void Project::clear_recent_projects() {
  recent_projects.clear();
  save_recent_projects();
}

void Project::set_icon_view_size(int s) {
  icon_view->setIconSize(QSize(s, s));
}

void Project::set_up_dir_enabled() {
  directory_up->setEnabled(icon_view->rootIndex().isValid());
}

void Project::go_up_dir() {
  icon_view->setRootIndex(icon_view->rootIndex().parent());
  set_up_dir_enabled();
}

void Project::make_new_menu() {
  QMenu new_menu(this);
  olive::MenuHelper.make_new_menu(&new_menu);
  new_menu.exec(QCursor::pos());
}

void Project::add_recent_project(QString url) {
  bool found = false;
  for (int i=0;i<recent_projects.size();i++) {
    if (url == recent_projects.at(i)) {
      found = true;
      recent_projects.move(i, 0);
      break;
    }
  }
  if (!found) {
    recent_projects.insert(0, url);
    if (recent_projects.size() > MAXIMUM_RECENT_PROJECTS) {
      recent_projects.removeLast();
    }
  }
  save_recent_projects();
}

void Project::list_all_sequences_worker(QVector<Media*>* list, Media* parent) {
  for (int i=0;i<olive::project_model.childCount(parent);i++) {
    Media* item = olive::project_model.child(i, parent);
    switch (item->get_type()) {
    case MEDIA_TYPE_SEQUENCE:
      list->append(item);
      break;
    case MEDIA_TYPE_FOLDER:
      list_all_sequences_worker(list, item);
      break;
    }
  }
}

QVector<Media*> Project::list_all_project_sequences() {
  QVector<Media*> list;
  list_all_sequences_worker(&list, nullptr);
  return list;
}

QModelIndexList Project::get_current_selected() {
  if (olive::CurrentConfig.project_view_type == PROJECT_VIEW_TREE) {
    return tree_view->selectionModel()->selectedRows();
  }
  return icon_view->selectionModel()->selectedIndexes();
}
