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

#include "project.h"

#include <QDir>
#include <QFileInfo>

#include "common/xmlutils.h"
#include "core.h"
#include "dialog/progress/progress.h"
#include "node/factory.h"
#include "render/diskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

Project::Project() :
  is_modified_(false),
  autorecovery_saved_(true)
{
  // Generate UUID for this project
  RegenerateUuid();

  // Folder root for project
  root_ = new Folder();
  root_->setParent(this);
  root_->SetLabel(tr("Root"));
  root_->SetCanBeDeleted(false);
  AddDefaultNode(root_);

  // Adds a color manager "node" to this project so that it synchronizes
  color_manager_ = new ColorManager();
  color_manager_->setParent(this);
  color_manager_->SetCanBeDeleted(false);
  AddDefaultNode(color_manager_);

  // Same with project settings
  settings_ = new ProjectSettingsNode();
  settings_->setParent(this);
  settings_->SetCanBeDeleted(false);
  AddDefaultNode(settings_);

  connect(color_manager(), &ColorManager::ValueChanged,
          this, &Project::ColorManagerValueChanged);
}

Folder *Project::root()
{
  return root_;
}

QString Project::name() const
{
  if (filename_.isEmpty()) {
    return tr("(untitled)");
  } else {
    return QFileInfo(filename_).completeBaseName();
  }
}

const QString &Project::filename() const
{
  return filename_;
}

QString Project::pretty_filename() const
{
  QString fn = filename();

  if (fn.isEmpty()) {
    return tr("(untitled)");
  } else {
    return fn;
  }
}

void Project::set_filename(const QString &s)
{
  filename_ = s;

#ifdef Q_OS_WINDOWS
  // Prevents filenames
  filename_.replace('/', '\\');
#endif

  emit NameChanged();
}

void Project::set_modified(bool e)
{
  is_modified_ = e;
  set_autorecovery_saved(!e);

  emit ModifiedChanged(is_modified_);
}

bool Project::has_autorecovery_been_saved() const
{
  return autorecovery_saved_;
}

void Project::set_autorecovery_saved(bool e)
{
  autorecovery_saved_ = e;
}

bool Project::is_new() const
{
  return !is_modified_ && filename_.isEmpty();
}

QString Project::get_cache_alongside_project_path() const
{
  if (!filename_.isEmpty()) {
    // Non-translated string so the path doesn't change if the language does
    return QFileInfo(filename_).dir().filePath(QStringLiteral("cache"));
  }
  return QString();
}

QString Project::cache_path() const
{
  ProjectSettingsNode::CacheSetting setting = settings_->GetCacheSetting();

  switch (setting) {
  case ProjectSettingsNode::kCacheUseDefaultLocation:
    break;
  case ProjectSettingsNode::kCacheCustomPath:
  {
    QString cache_path = settings_->GetCustomCachePath();
    if (cache_path.isEmpty()) {
      return cache_path;
    }
    break;
  }
  case ProjectSettingsNode::kCacheStoreAlongsideProject:
  {
    QString alongside = get_cache_alongside_project_path();
    if (!alongside.isEmpty()) {
      return alongside;
    }
    break;
  }
  }

  return DiskManager::instance()->GetDefaultCachePath();
}

void Project::RegenerateUuid()
{
  uuid_ = QUuid::createUuid();
}

void Project::ColorManagerValueChanged(const NodeInput &input, const TimeRange &range)
{
  Q_UNUSED(input)
  Q_UNUSED(range)

  QVector<Footage*> footage = root()->ListChildrenOfType<Footage>();

  foreach (Footage* item, footage) {
    item->InvalidateAll(QString());
  }
}

}
