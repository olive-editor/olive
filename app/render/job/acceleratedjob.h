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

#ifndef ACCELERATEDJOB_H
#define ACCELERATEDJOB_H

#include "node/input.h"
#include "node/inputarray.h"
#include "render/shadervalue.h"
#include "node/value.h"

OLIVE_NAMESPACE_ENTER

class AcceleratedJob {
public:
  AcceleratedJob() = default;

  ShaderValue GetValue(NodeInput* input) const
  {
    return value_map_.value(input->id());
  }

  ShaderValue GetValue(const QString& input) const
  {
    return value_map_.value(input);
  }

  void InsertValue(NodeInput* input, NodeValueDatabase& value)
  {
    ShaderValue shader_val;

    shader_val.type = input->data_type();
    shader_val.array = input->IsArray();

    if (input->IsArray()) {
      NodeInputArray* array = static_cast<NodeInputArray*>(input);
      QVector<QVariant> values(array->GetSize());

      for (int j=0;j<array->GetSize();j++) {
        NodeInput* subparam = array->At(j);

        values[j] = value[subparam].Take(subparam->data_type());
      }

      shader_val.data = QVariant::fromValue(values);
    } else {
      NodeValue node_val = value[input].TakeWithMeta(input->data_type());
      shader_val.data = node_val.data();
      shader_val.tag = node_val.tag();
    }

    InsertValue(input->id(), shader_val);
  }

  void InsertValue(const QString& input, const ShaderValue& value)
  {
    value_map_.insert(input, value);
  }

  void InsertValue(NodeInput* input, const ShaderValue& value)
  {
    value_map_.insert(input->id(), value);
  }

  void InsertValue(NodeInput* input, const NodeValue& value)
  {
    ShaderValue s(value.data(), value.type());
    s.tag = value.tag();
    value_map_.insert(input->id(), s);
  }

  const NodeValueMap &GetValues() const
  {
    return value_map_;
  }

private:
  NodeValueMap value_map_;

};

OLIVE_NAMESPACE_EXIT

#endif // ACCELERATEDJOB_H
