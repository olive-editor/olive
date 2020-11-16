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

#ifndef VOLUMENODE_H
#define VOLUMENODE_H

#include "node/math/math/mathbase.h"

OLIVE_NAMESPACE_ENTER

class VolumeNode : public MathNodeBase
{
public:
  VolumeNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual NodeValueTable Value(NodeValueDatabase &value) const override;

  virtual void ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const override;

  virtual void Retranslate() override;

  NodeInput* samples_input() const
  {
    return samples_input_;
  }

private:
  NodeInput* samples_input_;
  NodeInput* volume_input_;

};

OLIVE_NAMESPACE_EXIT

#endif // VOLUMENODE_H
