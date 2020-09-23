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

#ifndef CRASHPADUTILS_H
#define CRASHPADUTILS_H

#include <client/crashpad_client.h>

// Copied from base::FilePath to match its macro
#if defined(OS_POSIX)
// On most platforms, native pathnames are char arrays, and the encoding
// may or may not be specified.  On Mac OS X, native pathnames are encoded
// in UTF-8.
#define QSTRING_TO_BASE_STRING(x) x.toStdString()
#define BASE_STRING_TO_QSTRING(x) QString::fromStdString(x)
#elif defined(OS_WIN)
// On Windows, for Unicode-aware applications, native pathnames are wchar_t
// arrays encoded in UTF-16.
#define QSTRING_TO_BASE_STRING(x) x.toStdWString()
#define BASE_STRING_TO_QSTRING(x) QString::fromStdWString(x)
#endif  // OS_WIN

#endif // CRASHPADUTILS_H
