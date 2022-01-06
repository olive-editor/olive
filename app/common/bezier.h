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

#ifndef BEZIER_H
#define BEZIER_H

#include <QPointF>

#include "common/define.h"

namespace olive {

class Bezier
{
public:
  static double QuadraticXtoT(double x, double a, double b, double c);

  static double QuadraticTtoY(double a, double b, double c, double t);

  static double QuadraticXtoY(double x, const QPointF &a, const QPointF &b, const QPointF &c)
  {
    return QuadraticTtoY(a.y(), b.y(), c.y(), QuadraticXtoT(x, a.x(), b.x(), c.x()));
  }

  static double CubicXtoT(double x, double a, double b, double c, double d);

  static double CubicTtoY(double a, double b, double c, double d, double t);

  static double CubicXtoY(double x, const QPointF &a, const QPointF &b, const QPointF &c, const QPointF &d)
  {
    return CubicTtoY(a.y(), b.y(), c.y(), d.y(), CubicXtoT(x, a.x(), b.x(), c.x(), d.x()));
  }

private:
  static double CalculateTFromX(bool cubic, double x, double a, double b, double c, double d);

};

}

#endif // BEZIER_H
