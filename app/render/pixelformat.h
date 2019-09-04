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

#ifndef BITDEPTHS_H
#define BITDEPTHS_H

namespace olive {

/**
 * @brief Olive's internal supported pixel formats.
 */
enum PixelFormat {
  PIX_FMT_INVALID = -1,

  PIX_FMT_RGBA8,
  PIX_FMT_RGBA16U,
  PIX_FMT_RGBA16F,
  PIX_FMT_RGBA32F,

  PIX_FMT_COUNT
};

}

#endif // BITDEPTHS_H
