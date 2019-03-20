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

#ifndef BITDEPTHS_H
#define BITDEPTHS_H

#include <QString>
#include <QVector>
#include <QOpenGLExtraFunctions>

namespace olive {
  namespace rendering {
    struct BitDepthInfo {
      QString name;
      GLuint pixel_type;
      GLuint internal_format;
    };

    extern QVector<BitDepthInfo> bit_depths;

    void InitializeBitDepths();
  }
}

#endif // BITDEPTHS_H
