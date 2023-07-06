/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef WAVEDISTORTNODE_H
#define WAVEDISTORTNODE_H

#include "node/node.h"

namespace olive {

class WaveDistortNode : public Node
{
  Q_OBJECT
public:
  WaveDistortNode();

  NODE_DEFAULT_FUNCTIONS(WaveDistortNode)

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual value_t Value(const ValueParams &globals) const override;

  static const QString kTextureInput;
  static const QString kFrequencyInput;
  static const QString kIntensityInput;
  static const QString kEvolutionInput;
  static const QString kVerticalInput;

private:
  static ShaderCode GetShaderCode(const QString &id);

};

}

#endif // WAVEDISTORTNODE_H
