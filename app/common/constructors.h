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

#ifndef CONSTRUCTORS_H
#define CONSTRUCTORS_H

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

/**
 * Copy/move deleters. Similar to Q_DISABLE_COPY_MOVE, et al. but those functions are not present in Qt < 5.13 so we
 * use our own functions for portability.
 */

#define DISABLE_COPY(Class) \
  Class(const Class &) = delete;\
  Class &operator=(const Class &) = delete;

#define DISABLE_MOVE(Class) \
  Class(Class &&) = delete; \
  Class &operator=(Class &&) = delete;

#define DISABLE_COPY_MOVE(Class) \
  DISABLE_COPY(Class) \
  DISABLE_MOVE(Class)

OLIVE_NAMESPACE_EXIT

#endif // CONSTRUCTORS_H
