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

#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <memory>

#include "node/project/projectsettings/projectsettings.h"
#include "render/colormanager.h"
#include "project/item/folder/folder.h"
#include "window/mainwindow/mainwindowlayoutinfo.h"

namespace olive {

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
class Project : public NodeGraph
{
  Q_OBJECT
public:
  Project();

  void Load(QXmlStreamReader* reader, MainWindowLayoutInfo *layout, uint version, const QAtomicInt* cancelled);

  void Save(QXmlStreamWriter* writer) const;

  Folder* root();

  QString name() const;

  const QString& filename() const;
  QString pretty_filename() const;
  void set_filename(const QString& s);

  ColorManager* color_manager();

  bool is_modified() const;
  void set_modified(bool e);

  bool has_autorecovery_been_saved() const;
  void set_autorecovery_saved(bool e);

  bool is_new() const;

  QString cache_path() const;

signals:
  void NameChanged();

  void ModifiedChanged(bool e);

private:
  Folder* root_;

  QString filename_;

  ColorManager* color_manager_;

  ProjectSettingsNode* settings_;

  bool is_modified_;

  bool autorecovery_saved_;

private slots:
  void ColorManagerValueChanged(const NodeInput& input, const TimeRange& range);

};

}

#endif // PROJECT_H
