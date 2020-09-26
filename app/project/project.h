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

#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <memory>

#include "render/colormanager.h"
#include "project/item/folder/folder.h"
#include "window/mainwindow/mainwindowlayoutinfo.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A project instance containing all the data pertaining to the user's project
 *
 * A project instance uses a parent-child hierarchy of Item objects. Projects will usually contain the following:
 *
 * * Footage
 * * Sequences
 * * Folders
 * * Project Settings
 * * Window Layout
 */
class Project : public QObject
{
  Q_OBJECT
public:
  Project();

  void Load(QXmlStreamReader* reader, MainWindowLayoutInfo *layout, const QAtomicInt* cancelled);

  void Save(QXmlStreamWriter* writer) const;

  Folder* root();

  QString name() const;

  const QString& filename() const;
  QString pretty_filename() const;
  void set_filename(const QString& s);

  ColorManager* color_manager();

  QList<ItemPtr> get_items_of_type(Item::Type type) const;

  bool is_modified() const;
  void set_modified(bool e);

  bool has_autorecovery_been_saved() const;
  void set_autorecovery_saved(bool e);

  bool is_new() const;

  const QString& cache_path(bool default_if_empty = true) const;

  void set_cache_path(const QString& cache_path)
  {
    cache_path_ = cache_path;
  }

signals:
  void NameChanged();

  void ModifiedChanged(bool e);

private:
  Folder root_;

  QString filename_;

  ColorManager color_manager_;

  bool is_modified_;

  bool autorecovery_saved_;

  QString cache_path_;

private slots:
  void ColorConfigChanged();

  void DefaultColorSpaceChanged();

};

using ProjectPtr = std::shared_ptr<Project>;

OLIVE_NAMESPACE_EXIT

#endif // PROJECT_H
