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

#ifndef COLORTRANSFORM_H
#define COLORTRANSFORM_H

#include <QString>

#include "common/define.h"
#include "common/ocioutils.h"

namespace olive {

class ColorTransform
{
public:
  ColorTransform()
  {
    is_display_ = false;
  }

  ColorTransform(const QString& output)
  {
    is_display_ = false;
    output_ = output;
  }

  ColorTransform(const QString& display, const QString& view, const QString& look)
  {
    is_display_ = true;
    output_ = display;
    view_ = view;
    look_ = look;
  }

  bool is_display() const {
    return is_display_;
  }

  const QString& display() const {
    return output_;
  }

  const QString& output() const {
    return output_;
  }

  const QString& view() const {
    return view_;
  }

  const QString& look() const {
    return look_;
  }

private:
  QString output_;

  bool is_display_;
  QString view_;
  QString look_;

};

}

#endif // COLORTRANSFORM_H
