/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef CLAMP_H
#define CLAMP_H

template<typename T>
/**
 * @brief Clamp a value between a minimum and a maximum value
 *
 * Similar to using min() and max() functions, but performs both at once. If value is less than minimum, this returns
 * minimum. If it is more than maximum, this returns maximum. Otherwise it returns value as-is.
 *
 * @return
 *
 * Will always return a value between minimum and maximum (inclusive).
 */
T clamp(T value, T minimum, T maximum) {
  if (value < minimum) {
    return minimum;
  }

  if (value > maximum) {
    return maximum;
  }

  return value;
}

#endif // CLAMP_H
