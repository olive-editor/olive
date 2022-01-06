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

#include <iostream>

#define OLIVE_TEST_SUCCESS -1

#define OLIVE_ASSERT(x) if (!(x)) return __LINE__
#define OLIVE_ASSERT_EQUAL(x, y) if (x != y) {std::cout << " - Equal assert failed: " << x << " != " << y; return __LINE__;}void()
#define OLIVE_TEST_END return OLIVE_TEST_SUCCESS

#define OLIVE_ADD_TEST(x) int Test##x()
#define OLIVE_ADD_DISABLED_TEST(x) int Test##x()
