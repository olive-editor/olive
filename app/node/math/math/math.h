/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef MATHNODE_H
#define MATHNODE_H

#include "mathbase.h"

namespace olive {

class MathNode : public MathNodeBase
{
  Q_OBJECT
public:
  MathNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;

  Operation GetOperation() const
  {
    return static_cast<Operation>(GetStandardValue(kMethodIn).toInt());
  }

  void SetOperation(Operation o)
  {
    SetStandardValue(kMethodIn, o);
  }

  virtual NodeValueTable Value(const QString& output, NodeValueDatabase &value) const override;

  virtual void ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const override;

  static const QString kMethodIn;
  static const QString kParamAIn;
  static const QString kParamBIn;
  static const QString kParamCIn;

};

}

#endif // MATHNODE_H
