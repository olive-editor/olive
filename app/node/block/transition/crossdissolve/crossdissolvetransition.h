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

#ifndef CROSSDISSOLVETRANSITION_H
#define CROSSDISSOLVETRANSITION_H

#include "node/block/transition/transition.h"

namespace olive {

class CrossDissolveTransition : public TransitionBlock
{
  Q_OBJECT
public:
  CrossDissolveTransition();

  NODE_DEFAULT_DESTRUCTOR(CrossDissolveTransition)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  //virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const QString& shader_id) const override;

protected:
  virtual void ShaderJobEvent(NodeValueDatabase& value, ShaderJob& job) const override;

  virtual void SampleJobEvent(SampleBufferPtr from_samples, SampleBufferPtr to_samples, SampleBufferPtr out_samples, double time_in) const override;

};

}

#endif // CROSSDISSOLVETRANSITION_H
