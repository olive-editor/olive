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

#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include <QString>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A collection of static file and directory functions
 */
class FileFunctions {
public:
  /**
   * @brief Returns true if the application is running in portable mode
   *
   * In portable mode, any persistent configuration files should be made in a path relative to the application rather
   * than in the user's home folder.
   */
  static bool IsPortable();

  static QString GetUniqueFileIdentifier(const QString& filename);

  static QString GetMediaIndexLocation();

  static QString GetMediaIndexFilename(const QString& filename);

  static QString GetMediaCacheLocation();

  static QString GetConfigurationLocation();

  static QString GetApplicationPath();

  static QString GetTempFilePath();

  static void CopyDirectory(const QString& source, const QString& dest);
};



OLIVE_NAMESPACE_EXIT

#endif // FILEFUNCTIONS_H
