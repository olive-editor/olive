/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef SHADERCODE_H
#define SHADERCODE_H

#include "common/filefunctions.h"

namespace olive {

class ShaderCode {
public:
  ShaderCode(const QString& frag_code = QString(), const QString& vert_code = QString()) :
    frag_code_(frag_code),
    vert_code_(vert_code)
  {
    if (frag_code_.isEmpty()) {
      frag_code_ = FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/default.frag"));
    }

    if (vert_code_.isEmpty()) {
      vert_code_ = FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/default.vert"));
    }
  }

  const QString& frag_code() const
  {
    return frag_code_;
  }

  const QString& vert_code() const
  {
    return vert_code_;
  }

private:
  QString frag_code_;

  QString vert_code_;

};

}

#endif // SHADERCODE_H
