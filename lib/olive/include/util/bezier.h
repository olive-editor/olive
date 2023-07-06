/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef LIBOLIVECORE_BEZIER_H
#define LIBOLIVECORE_BEZIER_H

#include <Imath/ImathVec.h>

namespace olive {

class Bezier
{
public:
  Bezier();
  Bezier(double x, double y);
  Bezier(double x, double y, double cp1_x, double cp1_y, double cp2_x, double cp2_y);

  const double &x() const {return x_; }
  const double &y() const {return y_; }
  const double &cp1_x() const { return cp1_x_; }
  const double &cp1_y() const { return cp1_y_; }
  const double &cp2_x() const { return cp2_x_; }
  const double &cp2_y() const { return cp2_y_; }

  Imath::V2d to_vec() const
  {
    return Imath::V2d(x_, y_);
  }

  Imath::V2d control_point_1_to_vec() const
  {
    return Imath::V2d(cp1_x_, cp1_y_);
  }

  Imath::V2d control_point_2_to_vec() const
  {
    return Imath::V2d(cp2_x_, cp2_y_);
  }

  void set_x(const double &x) { x_ = x; }
  void set_y(const double &y) { y_ = y; }
  void set_cp1_x(const double &cp1_x) { cp1_x_ = cp1_x; }
  void set_cp1_y(const double &cp1_y) { cp1_y_ = cp1_y; }
  void set_cp2_x(const double &cp2_x) { cp2_x_ = cp2_x; }
  void set_cp2_y(const double &cp2_y) { cp2_y_ = cp2_y; }

  static double QuadraticXtoT(double x, double a, double b, double c);

  static double QuadraticTtoY(double a, double b, double c, double t);

  static double QuadraticXtoY(double x, const Imath::V2d &a, const Imath::V2d &b, const Imath::V2d &c)
  {
    return QuadraticTtoY(a.y, b.y, c.y, QuadraticXtoT(x, a.x, b.x, c.x));
  }

  static double CubicXtoT(double x, double a, double b, double c, double d);

  static double CubicTtoY(double a, double b, double c, double d, double t);

  static double CubicXtoY(double x, const Imath::V2d &a, const Imath::V2d &b, const Imath::V2d &c, const Imath::V2d &d)
  {
    return CubicTtoY(a.y, b.y, c.y, d.y, CubicXtoT(x, a.x, b.x, c.x, d.x));
  }

private:
  static double CalculateTFromX(bool cubic, double x, double a, double b, double c, double d);

  double x_;
  double y_;

  double cp1_x_;
  double cp1_y_;

  double cp2_x_;
  double cp2_y_;

};

}

#endif // LIBOLIVECORE_BEZIER_H
