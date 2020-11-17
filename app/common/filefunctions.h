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

  static QString GetConfigurationLocation();

  static QString GetApplicationPath();

  static QString GetTempFilePath();

  static bool CanCopyDirectoryWithoutOverwriting(const QString& source, const QString& dest);

  static void CopyDirectory(const QString& source, const QString& dest, bool overwrite = false);

  static bool DirectoryIsValid(const QString& dir, bool try_to_create);

  /**
   * @brief Ensures a given filename has a certain extension
   *
   * Checks if the filename has the extension provided and appends it if not. The extension is
   * checked case-insensitive. The extension should be provided with no dot (e.g. "ove" rather than
   * ".ove").

   * @return The filename provided either untouched or with the extension appended to it.
   */
  static QString EnsureFilenameExtension(QString fn, const QString& extension);

  static QString ReadFileAsString(const QString& filename);

  /**
   * @brief Returns a temporary filename that can be used while writing rather than the original
   *
   * If overwriting a file, it's safest to write to a new file first and then only replace it at
   * the end so that if the program crashes or the user cancels the save half way through, the
   * original file is still intact.
   *
   * This function returns a slight variant of the filename provided that's guaranteed to not exist
   * and therefore won't overwrite anything important.
   */
  static QString GetSafeTemporaryFilename(const QString& original);

  /**
   * @brief Renames a file from `from` to `to`, deleting `to` if such a file already exists first
   */
  static bool RenameFileAllowOverwrite(const QString& from, const QString& to);

};



OLIVE_NAMESPACE_EXIT

#endif // FILEFUNCTIONS_H
