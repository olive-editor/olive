/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "styling.h"

#include "global/config.h"

bool olive::styling::UseDarkIcons()
{
  return olive::config.style == kOliveDefaultLight || olive::config.style == kNativeDarkIcons;
}

QColor olive::styling::GetIconColor()
{
  if (UseDarkIcons()) {
    return Qt::black;
  } else {
    return Qt::white;
  }
}



bool olive::styling::UseNativeUI()
{
  return olive::config.style == kNativeLightIcons || olive::config.style == kNativeDarkIcons;
}
