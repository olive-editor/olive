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

#include "mathfunctions.h"

namespace olive {

value_t Math::AddIntegerInteger(const value_t &a, const value_t &b)
{
  value_t v(TYPE_INTEGER, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<int64_t>(i) + b.value<int64_t>(i);
  }
  return v;
}

value_t Math::AddDoubleDouble(const value_t &a, const value_t &b)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) + b.value<double>(i);
  }
  return v;
}

value_t Math::SubtractDoubleDouble(const value_t &a, const value_t &b)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) - b.value<double>(i);
  }
  return v;
}

value_t Math::MultiplyDoubleDouble(const value_t &a, const value_t &b)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) * b.value<double>(i);
  }
  return v;
}

value_t Math::DivideDoubleDouble(const value_t &a, const value_t &b)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) / b.value<double>(i);
  }
  return v;
}

value_t Math::PowerDoubleDouble(const value_t &a, const value_t &b)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::pow(a.value<double>(i), b.value<double>(i));
  }
  return v;
}

value_t Math::AddMatrixMatrix(const value_t &a, const value_t &b)
{
  value_t v(TYPE_MATRIX, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<QMatrix4x4>(i) + b.value<QMatrix4x4>(i);
  }
  return v;
}

value_t Math::SubtractMatrixMatrix(const value_t &a, const value_t &b)
{
  value_t v(TYPE_MATRIX, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<QMatrix4x4>(i) - b.value<QMatrix4x4>(i);
  }
  return v;
}

value_t Math::MultiplyMatrixMatrix(const value_t &a, const value_t &b)
{
  value_t v(TYPE_MATRIX, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<QMatrix4x4>(i) * b.value<QMatrix4x4>(i);
  }
  return v;
}

}
