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

#ifndef OLIVECOMMONDEFINE_H
#define OLIVECOMMONDEFINE_H

#define OLIVE_NAMESPACE olive

#define OLIVE_NAMESPACE_ENTER namespace OLIVE_NAMESPACE {

#define OLIVE_NAMESPACE_EXIT }

OLIVE_NAMESPACE_ENTER

const int kHSVChannels = 3;
const int kRGBChannels = 3;
const int kRGBAChannels = 4;

/// The minimum size an icon in ProjectExplorer can be
const int kProjectIconSizeMinimum = 16;

/// The maximum size an icon in ProjectExplorer can be
const int kProjectIconSizeMaximum = 256;

/// The default size an icon in ProjectExplorer can be
const int kProjectIconSizeDefault = 64;

OLIVE_NAMESPACE_EXIT

#define MACRO_NAME_AS_STR(s) #s
#define MACRO_VAL_AS_STR(s) MACRO_NAME_AS_STR(s)

#define OLIVE_NS_CONST_ARG(x, y) QArgument<const OLIVE_NAMESPACE::x>("const " MACRO_VAL_AS_STR(OLIVE_NAMESPACE) "::" #x, y)
#define OLIVE_NS_ARG(x, y) QArgument<OLIVE_NAMESPACE::x>(MACRO_VAL_AS_STR(OLIVE_NAMESPACE) "::" #x, y)
#define OLIVE_NS_RETURN_ARG(x, y) QReturnArgument<OLIVE_NAMESPACE::x>(MACRO_VAL_AS_STR(OLIVE_NAMESPACE) "::" #x, y)

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

#endif // OLIVECOMMONDEFINE_H
