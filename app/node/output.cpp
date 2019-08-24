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

#include "output.h"

#include "node/node.h"

NodeOutput::NodeOutput(const QString &id) :
  NodeParam(id)
{
}

NodeParam::Type NodeOutput::type()
{
  return kOutput;
}

const NodeParam::DataType &NodeOutput::data_type()
{
  return data_type_;
}

void NodeOutput::set_data_type(const NodeParam::DataType &type)
{
  data_type_ = type;

  // If no name has been set, use a default name
  if (name().isEmpty()) {
    set_name(GetDefaultDataTypeName(type));
  }
}

const QVariant &NodeOutput::get_value(const rational& time)
{
  // Node::Process() should put the correct value in this output
  parent()->Run(time);

  // The value should be have been set by this point
  return value_;
}

void NodeOutput::set_value(const QVariant &value)
{
  value_ = value;
}
