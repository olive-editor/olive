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

#ifndef CONFIG_H
#define CONFIG_H

#include <QMap>
#include <QString>
#include <QVariant>

#include "common/timecodefunctions.h"

OLIVE_NAMESPACE_ENTER

class Config {
public:
  static Config& Current();

  void SetDefaults();

  static void Load();

  static void Save();

  QVariant operator[](const QString&) const;

  QVariant& operator[](const QString&);

private:
  Config();

  QMap<QString, QVariant> config_map_;

  static Config current_config_;

  static QString GetConfigFilePath();
};

OLIVE_NAMESPACE_EXIT

#endif // CONFIG_H
