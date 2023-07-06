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

value_t Math::AddDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) + b.value<double>(i);
  }
  return v;
}

value_t Math::SubtractDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) - b.value<double>(i);
  }
  return v;
}

value_t Math::MultiplyDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) * b.value<double>(i);
  }
  return v;
}

value_t Math::DivideDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<double>(i) / b.value<double>(i);
  }
  return v;
}

value_t Math::ModuloDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::fmod(a.value<double>(i), b.value<double>(i));
  }
  return v;
}

value_t Math::PowerDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::pow(a.value<double>(i), b.value<double>(i));
  }
  return v;
}

value_t Math::SineDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::sin(a.value<double>(i));
  }
  return v;
}

value_t Math::CosineDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::cos(a.value<double>(i));
  }
  return v;
}

value_t Math::TangentDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::tan(a.value<double>(i));
  }
  return v;
}

value_t Math::ArcSineDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::asin(a.value<double>(i));
  }
  return v;
}

value_t Math::ArcCosineDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::acos(a.value<double>(i));
  }
  return v;
}

value_t Math::ArcTangentDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::atan(a.value<double>(i));
  }
  return v;
}

value_t Math::HypSineDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::sinh(a.value<double>(i));
  }
  return v;
}

value_t Math::HypCosineDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::cosh(a.value<double>(i));
  }
  return v;
}

value_t Math::HypTangentDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::tanh(a.value<double>(i));
  }
  return v;
}

value_t Math::MinDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::min(a.value<double>(i), b.value<double>(i));
  }
  return v;
}

value_t Math::MaxDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::max(a.value<double>(i), b.value<double>(i));
  }
  return v;
}

value_t Math::ClampDoubleDoubleDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::clamp(a.value<double>(i), b.value<double>(i), c.value<double>(i));
  }
  return v;
}

value_t Math::FloorDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::floor(a.value<double>(i));
  }
  return v;
}

value_t Math::CeilDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::ceil(a.value<double>(i));
  }
  return v;
}

value_t Math::RoundDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::round(a.value<double>(i));
  }
  return v;
}

value_t Math::AbsDouble(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_DOUBLE, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = std::abs(a.value<double>(i));
  }
  return v;
}

value_t Math::AddMatrixMatrix(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_MATRIX, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<QMatrix4x4>(i) + b.value<QMatrix4x4>(i);
  }
  return v;
}

value_t Math::SubtractMatrixMatrix(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_MATRIX, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<QMatrix4x4>(i) - b.value<QMatrix4x4>(i);
  }
  return v;
}

value_t Math::MultiplyMatrixMatrix(const value_t &a, const value_t &b, const value_t &c)
{
  value_t v(TYPE_MATRIX, std::max(a.size(), b.size()));
  for (size_t i = 0; i < v.size(); i++) {
    v[i] = a.value<QMatrix4x4>(i) * b.value<QMatrix4x4>(i);
  }
  return v;
}

}
