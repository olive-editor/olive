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

#ifndef DIGIT_H
#define DIGIT_H

#include <stdint.h>

namespace olive {

inline int64_t GetDigitCount(int64_t input)
{
  input = std::abs(input);

  int64_t lim = 10;
  int64_t digit = 1;

  while (input >= lim) {
    lim *= 10;
    digit++;
  }

  return digit;
}

}

#endif // DIGIT_H
