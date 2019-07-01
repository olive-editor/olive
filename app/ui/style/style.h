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

#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QWidget>

namespace olive {
namespace style {

/**
 * @brief Sets the application style to "Olive Default"
 *
 * This function should probably be replaced (or at least renamed) later as alternative styles are reimplemented.
 * At the moment, this is a convenience function for setting to "Olive Default" which is a cross-platform default
 * style used in Olive.
 */
void AppSetDefault();

}
}

#endif // STYLEMANAGER_H
