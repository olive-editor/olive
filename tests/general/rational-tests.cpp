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

#include "testutil.h"

#include "common/rational.h"

namespace olive {

OLIVE_ADD_TEST(RationalDefaults)
{
  // By default, rationals are valid 0/1
  rational basic_constructor;
  OLIVE_ASSERT(basic_constructor.isNull());
  OLIVE_ASSERT(!basic_constructor.isNaN());

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(RationalNaN)
{
  // Create a NaN with a 0 denominator
  rational nan = rational(0, 0);
  OLIVE_ASSERT(nan.isNaN());
  OLIVE_ASSERT(nan.isNull());

  // Create a non-NaN with a zero numerator
  rational zero_nonnan(0, 999);
  OLIVE_ASSERT(zero_nonnan.isNull());
  OLIVE_ASSERT(!zero_nonnan.isNaN());

  // Create a non-NaN with a non-zero numerator
  rational nonzer_nonnan(1, 30);
  OLIVE_ASSERT(!nonzer_nonnan.isNull());
  OLIVE_ASSERT(!nonzer_nonnan.isNaN());

  OLIVE_TEST_END;
}

OLIVE_ADD_TEST(RationalNaNConstant)
{
  OLIVE_ASSERT(rational::NaN.isNaN());

  OLIVE_TEST_END;
}

}
