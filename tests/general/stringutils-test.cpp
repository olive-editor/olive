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

#include "util/stringutils.h"
#include "util/tests.h"

using namespace olive::core;

bool stringutils_format_test()
{
  const char *expected = "Hello, world!";
  std::string f = StringUtils::format("%s, %s!", "Hello", "world");
  if (strcmp(f.c_str(), expected) != 0) {
    return false;
  }

  return true;
}

int main()
{
  Tester t;

  t.add("StringUtils::format", stringutils_format_test);

  return t.exec();
}
