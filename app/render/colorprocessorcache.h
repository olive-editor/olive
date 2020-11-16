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

#ifndef COLORPROCESSORCACHE_H
#define COLORPROCESSORCACHE_H

#include "project/item/footage/stream.h"
#include "render/colorprocessor.h"

OLIVE_NAMESPACE_ENTER

using ColorProcessorCache = QHash<QString, ColorProcessorPtr>;

OLIVE_NAMESPACE_EXIT

#endif // COLORPROCESSORCACHE_H
