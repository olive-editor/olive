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

#include "managedcolor.h"

OLIVE_NAMESPACE_ENTER

ManagedColor::ManagedColor()
{
}

ManagedColor::ManagedColor(const double &r, const double &g, const double &b, const double &a) :
  Color(r, g, b, a)
{
}

ManagedColor::ManagedColor(const char *data, const VideoParams::Format &format, int channel_layout) :
  Color(data, format, channel_layout)
{
}

ManagedColor::ManagedColor(const Color &c) :
  Color(c)
{
}

const QString &ManagedColor::color_input() const
{
  return color_input_;
}

void ManagedColor::set_color_input(const QString &color_input)
{
  color_input_ = color_input;
}

const ColorTransform &ManagedColor::color_output() const
{
  return color_transform_;
}

void ManagedColor::set_color_output(const ColorTransform &color_output)
{
  color_transform_ = color_output;
}

OLIVE_NAMESPACE_EXIT
