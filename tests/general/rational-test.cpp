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

#include <cstring>

#include "util/rational.h"
#include "util/tests.h"

using namespace olive::core;

bool rational_to_from_string_test()
{
  rational r(1, 30);

  std::string s = r.toString();

  rational r2 = rational::fromString(s);

  return r == r2;
}

bool rational_to_from_string_test2()
{
  rational r(69, 420);

  std::string s = r.toString();

  rational r2 = rational::fromString(s);

  return r == r2;
}

bool rational_defaults()
{
  // By default, rationals are valid 0/1
  rational basic_constructor;

  if (!basic_constructor.isNull()) {
    return false;
  }

  if (basic_constructor.isNaN()) {
    return false;
  }

  return true;
}

bool rational_nan()
{
  // Create a NaN with a 0 denominator
  rational nan = rational(0, 0);
  if (!nan.isNaN()) return false;
  if (!nan.isNull()) return false;

  // Create a non-NaN with a zero numerator
  rational zero_nonnan(0, 999);
  if (!zero_nonnan.isNull()) return false;
  if (zero_nonnan.isNaN()) return false;

  // Create a non-NaN with a non-zero numerator
  rational nonzer_nonnan(1, 30);
  if (nonzer_nonnan.isNull()) return false;
  if (nonzer_nonnan.isNaN()) return false;

  return true;
}

bool rational_nan_constant()
{
  return rational::NaN.isNaN();
}

int main()
{
  Tester t;

  t.add("rational::defaults", rational_defaults);
  t.add("rational::NaN", rational_nan);
  t.add("rational::NaN_constant", rational_nan_constant);
  t.add("rational::toString/fromString", rational_to_from_string_test);
  t.add("rational::toString/fromString2", rational_to_from_string_test2);

  return t.exec();
}
