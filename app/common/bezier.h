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

#ifndef BEZIER_H
#define BEZIER_H

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class Bezier
{
public:
  static double QuadraticXtoT(double x, double a, double b, double c);

  static double QuadraticTtoY(double a, double b, double c, double t);

  static double CubicXtoT(double x_target, double a, double b, double c, double d);

  static double CubicTtoY(double a, double b, double c, double d, double t);
};

OLIVE_NAMESPACE_EXIT

#endif // BEZIER_H
