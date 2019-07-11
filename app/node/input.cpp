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

#include "input.h"

#include "output.h"

NodeInput::NodeInput(Node* parent) :
  NodeParam(parent),
  can_accept_multiple_inputs_(false)
{
  // Have at least one keyframe/value active at any time
  keyframes_.append(NodeKeyframe());
}

NodeParam::Type NodeInput::type()
{
  return kInput;
}

void NodeInput::add_data_input(const NodeParam::DataType &data_type)
{
  inputs_.append(data_type);
}

bool NodeInput::can_accept_type(const NodeParam::DataType &data_type)
{
  return AreDataTypesCompatible(data_type, inputs_);
}

bool NodeInput::can_accept_multiple_inputs()
{
  return can_accept_multiple_inputs_;
}

void NodeInput::set_can_accept_multiple_inputs(bool b)
{
  can_accept_multiple_inputs_ = b;
}

QVariant NodeInput::get_value(const rational &time)
{
  /// Determine if this input has any connections to it
  switch (edges_.size()) {

  /// No connections - use the internal value
  case 0:
    // FIXME: Re-implement keyframing
    return keyframes_.first().value();

  /// One connection - use the output of the connected Node
  case 1:
    return edges_.first()->output()->get_value(time);

  /// Multiple connections - rare, return a list of the outputs of the connected Nodes
  default:
  {
    QList<QVariant> values;

    for (int i=0;i<edges_.size();i++) {
      values.append(edges_.at(i)->output()->get_value(time));
    }

    return values;
  }
  }
}

bool NodeInput::keyframing()
{
  return keyframing_;
}

void NodeInput::set_keyframing(bool k)
{
  keyframing_ = k;
}
