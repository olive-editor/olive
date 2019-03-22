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

#ifndef PROJECT_H
#define PROJECT_H

#include <QVector>
#include <QTimer>
#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFile>
#include <QPushButton>

#include "project/projectmodel.h"
#include "project/projectfilter.h"
#include "project/projectelements.h"
#include "project/sourcescommon.h"
#include "ui/panel.h"
#include "ui/sourceiconview.h"
#include "timeline/mediaimportdata.h"
#include "undo/undo.h"

#include "ui/sourcetable.h"

#define LOAD_TYPE_VERSION 69
#define LOAD_TYPE_URL 70

extern QString autorecovery_filename;
extern QStringList recent_projects;

SequencePtr create_sequence_from_media(QVector<olive::timeline::MediaImportData> &media_list);

QString get_channel_layout_name(int channels, uint64_t layout);
QString get_interlacing_name(int interlacing);

class Project : public Panel {
  Q_OBJECT
public:
  explicit Project(QWidget *parent = nullptr);
  ~Project();

  bool is_focused();
  void clear();
  MediaPtr create_sequence_internal(ComboAction *ca, SequencePtr s, bool open, Media* parent);
  QString get_next_sequence_name(QString start = nullptr);
  void process_file_list(QStringList& files, bool recursive = false, MediaPtr replace = nullptr, Media *parent = nullptr);
  void replace_media(MediaPtr item, QString filename);
  Media* get_selected_folder();
  bool reveal_media(Media *media, QModelIndex parent = QModelIndex());
  void add_recent_project(QString url);

  void save_project(bool autorecovery);

  MediaPtr create_folder_internal(QString name);

  Media* item_to_media(const QModelIndex& index);
  MediaPtr item_to_media_ptr(const QModelIndex &index);

  void save_recent_projects();

  QVector<Media*> list_all_project_sequences();

  SourceTable* tree_view;
  SourceIconView* icon_view;
  SourcesCommon* sources_common;

  ProjectFilter* sorter;

  QVector<Media*> last_imported_media;

  QModelIndexList get_current_selected();

  void get_all_media_from_table(QList<Media *> &items, QList<Media *> &list, int type = -1);

  QWidget* toolbar_widget;

  virtual void Retranslate() override;
protected:
public slots:
  void import_dialog();
  void delete_selected_media();
  void duplicate_selected();
  void delete_clips_using_selected_media();
  void replace_selected_file();
  void replace_clip_media();
  void open_properties();
  void new_folder();
  void new_sequence();
private:
  void save_folder(QXmlStreamWriter& stream, int type, bool set_ids_only, const QModelIndex &parent = QModelIndex());
  int folder_id;
  int media_id;
  int sequence_id;
  void list_all_sequences_worker(QVector<Media *> *list, Media* parent);
  QString get_file_name_from_path(const QString &path);
  QDir proj_dir;
  QWidget* icon_view_container;
  QPushButton* directory_up;
  QLineEdit* toolbar_search;
private slots:
  void update_view_type();
  void set_icon_view();
  void set_tree_view();
  void clear_recent_projects();
  void set_icon_view_size(int);
  void set_up_dir_enabled();
  void go_up_dir();
  void make_new_menu();
};

#endif // PROJECT_H
