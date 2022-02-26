/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include <memory>
#include <QObject>
#include <QUuid>

#include "node/color/colormanager/colormanager.h"
#include "node/output/viewer/viewer.h"
#include "node/project/footage/footage.h"
#include "node/project/projectsettings/projectsettings.h"
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

  Folder* root();

  QString name() const;

  const QString& filename() const;
  QString pretty_filename() const;
  void set_filename(const QString& s);

  ColorManager* color_manager() { return color_manager_; }
  ProjectSettingsNode* settings() { return settings_; }

  bool is_modified() const { return is_modified_; }
  void set_modified(bool e);

  bool has_autorecovery_been_saved() const;
  void set_autorecovery_saved(bool e);

  bool is_new() const;

  QString get_cache_alongside_project_path() const;
  QString cache_path() const;

  const QUuid& GetUuid() const
  {
    return uuid_;
  }

  void SetUuid(const QUuid &uuid)
  {
    uuid_ = uuid;
  }

  void RegenerateUuid();

  const MainWindowLayoutInfo &GetLayoutInfo() const
  {
    return layout_info_;
  }

  void SetLayoutInfo(const MainWindowLayoutInfo &info)
  {
    layout_info_ = info;
  }

  /**
   * @brief Returns the filename the project was saved as, but not necessarily where it is now
   *
   * May help for resolving relative paths.
   */
  const QString &GetSavedURL() const
  {
    return saved_url_;
  }

  void SetSavedURL(const QString &url)
  {
    saved_url_ = url;
  }

signals:
  void NameChanged();

  void ModifiedChanged(bool e);

private:
  QUuid uuid_;

  Folder* root_;

  QString filename_;

  QString saved_url_;

  ColorManager* color_manager_;

  ProjectSettingsNode* settings_;

  bool is_modified_;

  bool autorecovery_saved_;

  MainWindowLayoutInfo layout_info_;

private slots:
  void ColorManagerValueChanged(const NodeInput& input, const TimeRange& range);

};

}

#endif // PROJECT_H
