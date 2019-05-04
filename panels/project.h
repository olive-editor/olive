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

class Project : public Panel {
  Q_OBJECT
public:
  explicit Project(QWidget *parent = nullptr);

  void ConnectFilterToModel();
  void DisconnectFilterToModel();

  virtual bool focused() override;

  Media* get_selected_folder();
  bool reveal_media(Media *media, QModelIndex parent = QModelIndex());

  Media* item_to_media(const QModelIndex& index);
  MediaPtr item_to_media_ptr(const QModelIndex &index);

  QModelIndexList get_current_selected();

  bool IsToolbarVisible();
  bool IsProjectWidget(QObject *child);

  virtual void Retranslate() override;
protected:
public slots:
  void delete_selected_media();
  void duplicate_selected();
  void delete_clips_using_selected_media();
  void replace_selected_file();
  void replace_clip_media();
  void open_properties();
  void new_folder();
  void SetToolbarVisible(bool visible);
private:
  QWidget* icon_view_container;
  QSlider* icon_size_slider;
  QPushButton* directory_up;
  QLineEdit* toolbar_search;

  QWidget* toolbar_widget;
  SourceTable* tree_view;
  SourceIconView* icon_view;

  ProjectFilter sorter;
  SourcesCommon sources_common;
private slots:
  void update_view_type();
  void set_icon_view();
  void set_list_view();
  void set_tree_view();
  void set_icon_view_size(int);
  void set_up_dir_enabled();
  void go_up_dir();
  void make_new_menu();
};

#endif // PROJECT_H
