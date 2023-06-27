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

#ifndef LIBOLIVECORE_TESTS_H
#define LIBOLIVECORE_TESTS_H

#include <list>

namespace olive {

class Tester
{
public:
  Tester() = default;

  typedef bool (*test_t)();

  void add(const char *name, test_t test_function)
  {
    test_names_.push_back(name);
    test_functions_.push_back(test_function);
  }

  bool run();

  int exec()
  {
    if (run()) {
      return 0;
    } else {
      return 1;
    }
  }

  static void echo(const char *fmt, ...);

private:
  std::list<const char*> test_names_;
  std::list<test_t> test_functions_;

};

}

#endif // LIBOLIVECORE_TESTS_H
