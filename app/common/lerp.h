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

#ifndef LERP_H
#define LERP_H

template<typename T>
/**
 * @brief Linearly interpolate a value between a and b using t
 *
 * t should be a number between 0.0 and 1.0. 0.0 will return a, 1.0 will return b, and between will return a value
 * in between a and b at that point linearly.
 */
T lerp(T a, T b, double t) {
  return (a * (1.0 - t)) + (b * t);
}

template<typename T>
T lerp(T a, T b, float t) {
  return (a * (1.0f - t)) + (b * t);
}

#endif // LERP_H
