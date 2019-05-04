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

#ifndef BLUR_H
#define BLUR_H

#include <QImage>
#include <QRect>

namespace olive {
  namespace ui {
    /**
     * @brief Convenience function for blurring a QImage
     *
     * @param result
     *
     * QImage to blur
     *
     * @param rect
     *
     * The rectangle of the QImage to blur. Use QImage::rect() to blur the entire image.
     *
     * @param radius
     *
     * The blur radius - how much to blur the image.
     *
     * @param alphaOnly
     *
     * True if only the alpha channel should be blurred rather than the entire RGBA space.
     */
    void blur(QImage& result, const QRect& rect, int radius, bool alphaOnly);
  }
}

#endif // BLUR_H
