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

#include "managedcolor.h"

OLIVE_NAMESPACE_ENTER

ManagedColor::ManagedColor()
{
}

ManagedColor::ManagedColor(const float &r, const float &g, const float &b, const float &a) :
  Color(r, g, b, a)
{
}

ManagedColor::ManagedColor(const char *data, const PixelFormat::Format &format) :
  Color(data, format)
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

const QString &ManagedColor::color_display() const
{
  return color_display_;
}

void ManagedColor::set_color_display(const QString &color_display)
{
  color_display_ = color_display;
}

const QString &ManagedColor::color_view() const
{
  return color_view_;
}

void ManagedColor::set_color_view(const QString &color_view)
{
  color_view_ = color_view;
}

const QString &ManagedColor::color_look() const
{
  return color_look_;
}

void ManagedColor::set_color_look(const QString &color_look)
{
  color_look_ = color_look;
}

OLIVE_NAMESPACE_EXIT
