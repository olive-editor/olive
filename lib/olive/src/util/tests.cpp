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

#include "util/tests.h"

#include <cstddef>
#include <cstdio>
#include <stdarg.h>

namespace olive {

bool Tester::run()
{
  size_t index = 1;
  size_t count = test_functions_.size();

  while (!test_functions_.empty()) {
    echo("[%lu/%lu] %s :: ", index, count, test_names_.front());

    if (test_functions_.front()()) {
      echo("PASSED\n");
    } else {
      echo("FAILED\n");
      return false;
    }

    test_names_.pop_front();
    test_functions_.pop_front();
  }

  return true;
}

void Tester::echo(const char *fmt, ...)
{
  va_list a;
  va_start(a, fmt);

  vfprintf(stderr, fmt, a);

  va_end(a);
}

}
