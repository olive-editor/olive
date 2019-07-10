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

#include "param.h"

#include "node.h"

NodeParam::NodeParam(Node *parent) :
  QObject(parent)
{

}

bool NodeParam::AreDataTypesCompatible(const NodeParam::DataType &output_type, const NodeParam::DataType &input_type)
{
  if (input_type == output_type) {
    return true;
  }

  if (input_type == kNone) {
    return false;
  }

  if (input_type == kAny) {
    return true;
  }

  // Allow for up-converting integers to floats (but not the other way around)
  if (output_type == kInt && input_type == kFloat) {
    return true;
  }

  return false;
}

bool NodeParam::AreDataTypesCompatible(const DataType &output_type, const QList<DataType>& input_types)
{
  for (int i=0;i<input_types.size();i++) {
    if (AreDataTypesCompatible(output_type, input_types.at(i))) {
      return true;
    }
  }

  return false;
}
