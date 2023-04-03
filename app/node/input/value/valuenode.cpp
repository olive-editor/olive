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

constexpr type_t OUR_DOUBLE = "dbl";
constexpr type_t OUR_INTEGER = "int";
constexpr type_t OUR_RATIONAL = "rational";
constexpr type_t OUR_VEC2 = "vec2";
constexpr type_t OUR_VEC3 = "vec3";
constexpr type_t OUR_VEC4 = "vec4";
constexpr type_t OUR_COLOR = "color";
constexpr type_t OUR_STRING = "str";
constexpr type_t OUR_MATRIX = "mat";
constexpr type_t OUR_FONT = "font";
constexpr type_t OUR_BOOL = "bool";

const QVector<ValueNode::Type> ValueNode::kSupportedTypes =
{
  {OUR_DOUBLE, TYPE_DOUBLE, 1},
  {OUR_INTEGER, TYPE_INTEGER, 1},
  {OUR_RATIONAL, TYPE_RATIONAL, 1},
  {OUR_VEC2, TYPE_DOUBLE, 2},
  {OUR_VEC3, TYPE_DOUBLE, 3},
  {OUR_VEC4, TYPE_DOUBLE, 4},
  {OUR_COLOR, TYPE_DOUBLE, 4},
  {OUR_STRING, TYPE_STRING, 1},
  {OUR_MATRIX, TYPE_MATRIX, 1},
  {OUR_FONT, TYPE_STRING, 1},
  {OUR_BOOL, TYPE_DOUBLE, 1},
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
  for (const Type &type : kSupportedTypes) {
    type_names.append(GetPrettyTypeName(type.our_type));
  }

  type_names.append(tr("Double"));
  type_names.append(tr("Integer"));
  type_names.append(tr("Rational"));
  type_names.append(tr("Vector 2D"));
  type_names.append(tr("Vector 3D"));
  type_names.append(tr("Vector 4D"));
  type_names.append(tr("Color"));
  type_names.append(tr("Text"));
  type_names.append(tr("Matrix"));
  type_names.append(tr("Font"));
  type_names.append(tr("Boolean"));

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

    SetInputDataType(kValueInput, kSupportedTypes.at(k).base_type, kSupportedTypes.at(k).channel_count);
  }

  super::InputValueChangedEvent(input, element);
}

QString ValueNode::GetPrettyTypeName(const type_t &id)
{
  if (id == OUR_DOUBLE) {
    return tr("Double");
  } else if (id == OUR_INTEGER) {
    return tr("Integer");
  } else if (id == OUR_RATIONAL) {
    return tr("Rational");
  } else if (id == OUR_VEC2) {
    return tr("Vector 2D");
  } else if (id == OUR_VEC3) {
    return tr("Vector 3D");
  } else if (id == OUR_VEC4) {
    return tr("Vector 4D");
  } else if (id == OUR_COLOR) {
    return tr("Color");
  } else if (id == OUR_STRING) {
    return tr("Text");
  } else if (id == OUR_MATRIX) {
    return tr("Matrix");
  } else if (id == OUR_FONT) {
    return tr("Font");
  } else if (id == OUR_BOOL) {
    return tr("Boolean");
  }

  return QString();
}

}
