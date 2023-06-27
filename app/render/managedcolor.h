/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef MANAGEDCOLOR_H
#define MANAGEDCOLOR_H

#include "colortransform.h"
#include "util/color.h"

namespace olive {

class ManagedColor : public Color
{
public:
  ManagedColor();
  ManagedColor(const double& r, const double& g, const double& b, const double& a = 1.0);
  ManagedColor(const char *data, const PixelFormat &format, int channel_layout);
  ManagedColor(const Color& c);

  const QString& color_input() const;
  void set_color_input(const QString &color_input);

  const ColorTransform& color_output() const;
  void set_color_output(const ColorTransform &color_output);

private:
  QString color_input_;

  ColorTransform color_transform_;

};

}

#endif // MANAGEDCOLOR_H
