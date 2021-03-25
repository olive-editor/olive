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

#include "projectsettings.h"

namespace olive {

const QString ProjectSettingsNode::kCacheSetting = QStringLiteral("cache_setting");
const QString ProjectSettingsNode::kCachePath = QStringLiteral("cache_path");

ProjectSettingsNode::ProjectSettingsNode()
{
  AddInput(kCacheSetting, NodeValue::kCombo, 0, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kCachePath, NodeValue::kFile, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));
  SetInputProperty(kCachePath, QStringLiteral("directory"), true);
  UpdateCachePathEnabled();
}

void ProjectSettingsNode::Retranslate()
{
  SetInputName(kCacheSetting, tr("Disk Cache Location"));
  SetInputName(kCachePath, tr("Disk Cache Path"));
  SetComboBoxStrings(kCacheSetting, {tr("Use Default Location"), tr("Store Alongside Project"), tr("Use Custom Location")});
  SetInputProperty(kCachePath, QStringLiteral("placeholder"), tr("(default)"));
}

void ProjectSettingsNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element)

  if (input == kCacheSetting) {
    UpdateCachePathEnabled();
  }
}

void ProjectSettingsNode::UpdateCachePathEnabled()
{
  CacheSetting setting = GetCacheSetting();
  SetInputProperty(kCachePath, QStringLiteral("enabled"), (setting == kCacheCustomPath));
}

}
