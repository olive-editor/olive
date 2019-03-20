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

#include "bitdepths.h"

#include <QCoreApplication>

namespace olive {
namespace rendering {

QVector<BitDepthInfo> bit_depths;

void InitializeBitDepths() {
  BitDepthInfo bdi;

  bdi.name = QCoreApplication::translate("bitdepths", "8-bit");
  bdi.pixel_type = GL_UNSIGNED_BYTE;
  bdi.internal_format = GL_RGBA8;
  bit_depths.append(bdi);

  bdi.name = QCoreApplication::translate("bitdepths", "Half-Float (16-bit)");
  bdi.pixel_type = GL_HALF_FLOAT;
  bdi.internal_format = GL_RGBA16F;
  bit_depths.append(bdi);

  bdi.name = QCoreApplication::translate("bitdepths", "Full-Float (32-bit)");
  bdi.pixel_type = GL_FLOAT;
  bdi.internal_format = GL_RGBA32F;
  bit_depths.append(bdi);
}

}
}

