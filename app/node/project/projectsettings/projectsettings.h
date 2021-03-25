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

#ifndef PROJECTSETTINGSNODE_H
#define PROJECTSETTINGSNODE_H

#include "node/node.h"

namespace olive {

class ProjectSettingsNode : public Node
{
  Q_OBJECT
public:
  ProjectSettingsNode();

  virtual QString Name() const override
  {
    return tr("Project Settings");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.projectsettings");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryProject};
  }

  virtual QString Description() const override
  {
    return tr("Settings used throughout the project.");
  }

  virtual Node* copy() const override
  {
    return new ProjectSettingsNode();
  }

  enum CacheSetting {
    kCacheUseDefaultLocation,
    kCacheStoreAlongsideProject,
    kCacheCustomPath
  };

  static const QString kCacheSetting;
  static const QString kCachePath;

  virtual void Retranslate() override;

  CacheSetting GetCacheSetting() const
  {
    return static_cast<CacheSetting>(GetStandardValue(kCacheSetting).toInt());
  }

  QString GetCustomCachePath() const
  {
    return GetStandardValue(kCachePath).toString();
  }

  void SetCustomCachePath(const QString& s)
  {
    SetStandardValue(kCachePath, s);
  }

protected:
  virtual void InputValueChangedEvent(const QString &input, int element) override;

private:
  void UpdateCachePathEnabled();

};

}

#endif // PROJECTSETTINGSNODE_H
