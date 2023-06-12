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

#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H

#include "node/value.h"

namespace olive {

class Math
{
public:
  static value_t AddDoubleDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t SubtractDoubleDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t MultiplyDoubleDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t DivideDoubleDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t PowerDoubleDouble(const value_t &a, const value_t &b, const value_t &c);

  static value_t SineDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t CosineDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t TangentDouble(const value_t &a, const value_t &b, const value_t &c);

  static value_t ArcSineDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t ArcCosineDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t ArcTangentDouble(const value_t &a, const value_t &b, const value_t &c);

  static value_t HypSineDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t HypCosineDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t HypTangentDouble(const value_t &a, const value_t &b, const value_t &c);

  static value_t MinDoubleDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t MaxDoubleDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t ClampDoubleDoubleDouble(const value_t &a, const value_t &b, const value_t &c);

  static value_t FloorDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t CeilDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t RoundDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t AbsDouble(const value_t &a, const value_t &b, const value_t &c);

  static value_t AddMatrixMatrix(const value_t &a, const value_t &b, const value_t &c);
  static value_t SubtractMatrixMatrix(const value_t &a, const value_t &b, const value_t &c);
  static value_t MultiplyMatrixMatrix(const value_t &a, const value_t &b, const value_t &c);

  static value_t MultiplySamplesDouble(const value_t &a, const value_t &b, const value_t &c);
  static value_t MultiplyDoubleSamples(const value_t &a, const value_t &b, const value_t &c);

};

}

#endif // MATHFUNCTIONS_H
