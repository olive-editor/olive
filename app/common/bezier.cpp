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

#include "bezier.h"

#include <QtMath>

namespace olive {

double Bezier::QuadraticXtoT(double x, double a, double b, double c)
{
  return (a - b + qSqrt(a*x + c*x - 2*b*x + qPow(b, 2) - a*c))/(a - 2*b + c);
}

double Bezier::QuadraticTtoY(double a, double b, double c, double t)
{
  return qPow(1.0 - t, 2)*a + 2*(1.0 - t)*t*b + qPow(t, 2)*c;
}

double Bezier::CubicXtoT(double x_target, double a, double b, double c, double d)
{
  const double tolerance = 0.0001;

  double lower = 0.0;
  double upper = 1.0;

  double percent = 0.5;
  double x = CubicTtoY(a, b, c, d, percent);

  while (qAbs(x_target - x) > tolerance) {
    if (x_target > x) {
      lower = percent;
    } else {
      upper = percent;
    }

    percent = (upper + lower) * 0.5;
    x = CubicTtoY(a, b, c, d, percent);
  }

  return percent;
}

double Bezier::CubicTtoY(double a, double b, double c, double d, double t)
{
  return qPow(1.0 - t, 3)*a + 3*qPow(1.0 - t, 2)*t*b + 3*(1.0 - t)*qPow(t, 2)*c + qPow(t, 3)*d;
}

}
