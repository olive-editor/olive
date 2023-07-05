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

#define super Node

const QVector<type_t> ValueNode::kSupportedTypes = {
  TYPE_DOUBLE,
  TYPE_INTEGER,
  TYPE_RATIONAL,
  TYPE_VEC2,
  TYPE_VEC3,
  TYPE_VEC4,
  TYPE_COLOR,
  TYPE_STRING,
  TYPE_MATRIX,
  TYPE_FONT,
  TYPE_BOOL
};

ValueNode::ValueNode()
{
  AddInput(kTypeInput, TYPE_COMBO, 0, kInputFlagNotConnectable | kInputFlagNotKeyframable);

  AddInput(kValueInput, TYPE_DOUBLE, kInputFlagNotConnectable);
}

void ValueNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTypeInput, QStringLiteral("Type"));
  SetInputName(kValueInput, QStringLiteral("Value"));

  QStringList type_names;
  type_names.reserve(kSupportedTypes.size());
  for (const type_t &type : kSupportedTypes) {
    type_names.append(GetPrettyTypeName(type));
  }

  SetComboBoxStrings(kTypeInput, type_names);
}

value_t ValueNode::Value(const ValueParams &p) const
{
  Q_UNUSED(p)

  // Ensure value is pushed onto the table
  return GetInputValue(p, kValueInput);
}

void ValueNode::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kTypeInput) {
    int64_t k = GetStandardValue(kTypeInput).toInt();

    const type_t &t = kSupportedTypes.at(k);

    SetInputDataType(kValueInput, t);
  }

  super::InputValueChangedEvent(input, element);
}

QString ValueNode::GetPrettyTypeName(const type_t &id)
{
  if (id == TYPE_DOUBLE) {
    return tr("Double");
  } else if (id == TYPE_INTEGER) {
    return tr("Integer");
  } else if (id == TYPE_RATIONAL) {
    return tr("Rational");
  } else if (id == TYPE_VEC2) {
    return tr("Vector 2D");
  } else if (id == TYPE_VEC3) {
    return tr("Vector 3D");
  } else if (id == TYPE_VEC4) {
    return tr("Vector 4D");
  } else if (id == TYPE_COLOR) {
    return tr("Color");
  } else if (id == TYPE_STRING) {
    return tr("Text");
  } else if (id == TYPE_MATRIX) {
    return tr("Matrix");
  } else if (id == TYPE_FONT) {
    return tr("Font");
  } else if (id == TYPE_BOOL) {
    return tr("Boolean");
  }

  return QString();
}

}
