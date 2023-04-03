/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "node/value.h"

namespace olive {

#define OLIVE_CONFIG(x) Config::Current()[QStringLiteral(x)]
#define OLIVE_CONFIG_STR(x) Config::Current()[x]

class Config
{
public:
  static Config& Current();

  void SetDefaults();

  static void Load();

  static void Save();

  value_t operator[](const QString&) const;

  value_t &operator[](const QString&);

private:
  Config();

  QMap<QString, value_t> config_map_;

  static Config current_config_;

  static QString GetConfigFilePath();
};

}

#endif // CONFIG_H
