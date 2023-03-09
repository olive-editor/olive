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

#ifndef DECIBEL_H
#define DECIBEL_H

#include <QtGlobal>
#include <cmath>

//#define ALLOW_RETURNING_INFINITY

namespace olive {

class Decibel
{
public:
  // In basically all circumstances, this should calculate to 0.0 linear
  static constexpr double MINIMUM = -200.0;

  static double fromLinear(double linear)
  {
    double v = double(20.0) * std::log10(linear);
#ifndef ALLOW_RETURNING_INFINITY
    if (std::isinf(v)) {
      return MINIMUM;
    }
#endif
    return v;
  }

  static double toLinear(double decibel)
  {
    double to_linear = std::pow(double(10.0), decibel / double(20.0));

    // Minimum threshold that we figure is close enough to 0 that we may as well just return 0
    if (to_linear < 0.000001) {
      return 0;
    } else {
      return to_linear;
    }
  }

  static double fromLogarithmic(double logarithmic)
  {
    if (logarithmic < 0.001)
#ifdef ALLOW_RETURNING_INFINITY
      return std::numeric_limits<double>::infinity();
#else
      return MINIMUM;
#endif
    else if (logarithmic > 0.99)
      return 0;
    else
      return 20.0 * std::log10(-std::log(1 - logarithmic) / LOG100);
  }

  static double toLogarithmic(double decibel)
  {
    if (qFuzzyIsNull(decibel)) {
      return 1;
    } else {
      return 1 - std::exp(-std::pow(10.0, decibel / 20.0) * LOG100);
    }
  }

  static double LinearToLogarithmic(double linear)
  {
    return 1 - std::exp(-linear * LOG100);
  }

  static double LogarithmicToLinear(double logarithmic)
  {
    if (logarithmic > 0.99) {
      return 1;
    } else {
      return -std::log(1 - logarithmic) / LOG100;
    }
  }

private:
  static constexpr double LOG100 = 4.60517018599;

};

}

#endif // DECIBEL_H
