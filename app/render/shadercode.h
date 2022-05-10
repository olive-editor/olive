/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
  }

  const QString& frag_code() const { return frag_code_; }
  void set_frag_code(const QString &f) { frag_code_ = f; }

  const QString& vert_code() const { return vert_code_; }
  void set_vert_code(const QString &v) { vert_code_ = v; }

private:
  QString frag_code_;

  QString vert_code_;

};

}

#endif // SHADERCODE_H
