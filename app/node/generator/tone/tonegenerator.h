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

#ifndef TONEGENERATOR_H
#define TONEGENERATOR_H

#include "node/node.h"

namespace olive {

class ToneGenerator : public Node
{
public:
  ToneGenerator();

  NODE_DEFAULT_FUNCTIONS(ToneGenerator)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual value_t Value(const ValueParams &p) const override;

  static const QString kFrequencyInput;

};

}

#endif // TONEGENERATOR_H
