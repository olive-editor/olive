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

#include "valuenode.h"

namespace olive {

const QString ValueNode::kTypeInput = QStringLiteral("type_in");
const QString ValueNode::kValueInput = QStringLiteral("value_in");
const QVector<NodeValue::Type> ValueNode::kSupportedTypes = {
  NodeValue::kFloat,
  NodeValue::kInt,
  NodeValue::kRational,
  NodeValue::kVec2,
  NodeValue::kVec3,
  NodeValue::kVec4,
  NodeValue::kColor,
  NodeValue::kText,
  NodeValue::kMatrix,
  NodeValue::kFont,
  NodeValue::kBoolean,
};

#define super Node

ValueNode::ValueNode()
{
  AddInput(kTypeInput, NodeValue::kCombo, 0, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kValueInput, kSupportedTypes.first(), QVariant(), InputFlags(kInputFlagNotConnectable));
}

void ValueNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTypeInput, QStringLiteral("Type"));
  SetInputName(kValueInput, QStringLiteral("Value"));

  QStringList type_names;
  type_names.reserve(kSupportedTypes.size());
  foreach (NodeValue::Type type, kSupportedTypes) {
    type_names.append(NodeValue::GetPrettyDataTypeName(type));
  }
  SetComboBoxStrings(kTypeInput, type_names);
}

NodeValue ValueNode::Value(const ValueParams &p) const
{
  Q_UNUSED(p)

  // Ensure value is pushed onto the table
  return GetInputValue(p, kValueInput);
}

void ValueNode::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kTypeInput) {
    SetInputDataType(kValueInput, kSupportedTypes.at(GetStandardValue(kTypeInput).toInt()));
  }

  super::InputValueChangedEvent(input, element);
}

}
