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

NodeInput::NodeInput(const QString& id) :
  NodeParam(id),
  time_(-1),
  keyframing_(false)
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

  // If no name has been set, use a default name
  if (name().isEmpty()) {
    set_name(GetDefaultDataTypeName(data_type));
  }
}

bool NodeInput::can_accept_type(const NodeParam::DataType &data_type)
{
  return AreDataTypesCompatible(data_type, inputs_);
}

QVariant NodeInput::get_value(const rational& time)
{
  if (time_ != time) {
    // Retrieve the value
    if (!edges_.isEmpty()) {
      // One connection - use the output of the connected Node
      value_ = edges_.first()->output()->get_value(time);
    }

    // No connections - use the internal value
    // FIXME: Re-implement keyframing
    value_ = keyframes_.first().value();

    time_ = time;
  }

  return value_;
}

void NodeInput::set_value(const QVariant &value)
{
  if (keyframing()) {
    // FIXME: Keyframing code using time()
  } else {
    keyframes_.first().set_value(value);

    // FIXME: Put correct values here
    emit ValueChanged(0, 0);
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

const QList<NodeParam::DataType> &NodeInput::inputs()
{
  return inputs_;
}
