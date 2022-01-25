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

#include "bezier.h"

#include <QtMath>

#include "common/clamp.h"

namespace olive {

Bezier::Bezier() :
  x_(0),
  y_(0),
  cp1_x_(0),
  cp1_y_(0),
  cp2_x_(0),
  cp2_y_(0)
{
}

Bezier::Bezier(double x, double y) :
  x_(x),
  y_(y),
  cp1_x_(0),
  cp1_y_(0),
  cp2_x_(0),
  cp2_y_(0)
{
}

Bezier::Bezier(double x, double y, double cp1_x, double cp1_y, double cp2_x, double cp2_y) :
  x_(x),
  y_(y),
  cp1_x_(cp1_x),
  cp1_y_(cp1_y),
  cp2_x_(cp2_x),
  cp2_y_(cp2_y)
{
}

double Bezier::QuadraticXtoT(double x, double a, double b, double c)
{
  // Clamp to prevent infinite loop
  x = clamp(x, a, c);

  return CalculateTFromX(false, x, a, b, c, 0);
}

double Bezier::QuadraticTtoY(double a, double b, double c, double t)
{
  return qPow(1.0 - t, 2)*a + 2*(1.0 - t)*t*b + qPow(t, 2)*c;
}

double Bezier::CubicXtoT(double x, double a, double b, double c, double d)
{
  // Clamp to prevent infinite loop
  x = clamp(x, a, d);

  return CalculateTFromX(true, x, a, b, c, d);
}

double Bezier::CubicTtoY(double a, double b, double c, double d, double t)
{
  return qPow(1.0 - t, 3)*a + 3*qPow(1.0 - t, 2)*t*b + 3*(1.0 - t)*qPow(t, 2)*c + qPow(t, 3)*d;
}

double Bezier::CalculateTFromX(bool cubic, double x, double a, double b, double c, double d)
{
  double bottom = 0.0;
  double top = 1.0;

  while (true) {
    if (bottom == top) {
      return bottom;
    }

    double mid = (bottom + top) * 0.5;
    double test = cubic ? CubicTtoY(a, b, c, d, mid) : QuadraticTtoY(a, b, c, mid);

    if (qAbs(test - x) < 0.000001) {
      return mid;
    } else if (x > test) {
      bottom = mid;
    } else {
      top = mid;
    }
  }

  return qSNaN();
}

}
