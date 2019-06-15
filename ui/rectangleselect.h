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

#ifndef RECTANGLESELECT_H
#define RECTANGLESELECT_H

#include <QPainter>

namespace olive {
namespace ui {

/**
 * @brief Routine for drawing a drag selection rectangle for any given QPainter
 *
 * @param painter
 *
 * QPainter object to use for drawing
 *
 * @param rect
 *
 * Rectangle to draw
 */
void DrawSelectionRectangle(QPainter& painter, const QRect& rect);

}
}

#endif // RECTANGLESELECT_H
