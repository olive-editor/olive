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

#include "common/digit.h"

namespace olive {

OLIVE_ADD_TEST(DigitTest)
{
  OLIVE_ASSERT(GetDigitCount(1) == 1);
  OLIVE_ASSERT(GetDigitCount(69) == 2);
  OLIVE_ASSERT(GetDigitCount(420) == 3);
  OLIVE_ASSERT(GetDigitCount(1337) == 4);
  OLIVE_ASSERT(GetDigitCount(80085) == 5);
  OLIVE_ASSERT(GetDigitCount(555555) == 6);
  OLIVE_ASSERT(GetDigitCount(8675309) == 7);
  OLIVE_ASSERT(GetDigitCount(78956423) == 8);
  OLIVE_ASSERT(GetDigitCount(148497523) == 9);
  OLIVE_ASSERT(GetDigitCount(4845821233) == 10);
  OLIVE_ASSERT(GetDigitCount(18002738255) == 11);
  OLIVE_ASSERT(GetDigitCount(180027382556) == 12);
  OLIVE_ASSERT(GetDigitCount(1800273825568) == 13);
  OLIVE_ASSERT(GetDigitCount(18002738255685) == 14);
  OLIVE_ASSERT(GetDigitCount(180027382556857) == 15);
  OLIVE_ASSERT(GetDigitCount(1800273825564857) == 16);

  OLIVE_TEST_END;
}

}
