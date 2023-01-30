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

#include "trigonometry.h"

namespace olive {

const QString TrigonometryNode::kMethodIn = QStringLiteral("method_in");
const QString TrigonometryNode::kXIn = QStringLiteral("x_in");

#define super Node

TrigonometryNode::TrigonometryNode()
{
  AddInput(kMethodIn, NodeValue::kCombo, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kXIn, NodeValue::kFloat, 0.0);
}

QString TrigonometryNode::Name() const
{
  return tr("Trigonometry");
}

QString TrigonometryNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.trigonometry");
}

QVector<Node::CategoryID> TrigonometryNode::Category() const
{
  return {kCategoryMath};
}

QString TrigonometryNode::Description() const
{
  return tr("Perform a trigonometry operation on a value.");
}

void TrigonometryNode::Retranslate()
{
  super::Retranslate();

  QStringList strings = {tr("Sine"),
                         tr("Cosine"),
                         tr("Tangent"),
                         QString(),
                         tr("Inverse Sine"),
                         tr("Inverse Cosine"),
                         tr("Inverse Tangent"),
                         QString(),
                         tr("Hyperbolic Sine"),
                         tr("Hyperbolic Cosine"),
                         tr("Hyperbolic Tangent")};

  SetComboBoxStrings(kMethodIn, strings);

  SetInputName(kMethodIn, tr("Method"));

  SetInputName(kXIn, tr("Value"));
}

void TrigonometryNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  double x = value[kXIn].toDouble();

  switch (static_cast<Operation>(GetStandardValue(kMethodIn).toInt())) {
  case kOpSine:
    x = std::sin(x);
    break;
  case kOpCosine:
    x = std::cos(x);
    break;
  case kOpTangent:
    x = std::tan(x);
    break;
  case kOpArcSine:
    x = std::asin(x);
    break;
  case kOpArcCosine:
    x = std::acos(x);
    break;
  case kOpArcTangent:
    x = std::atan(x);
    break;
  case kOpHypSine:
    x = std::sinh(x);
    break;
  case kOpHypCosine:
    x = std::cosh(x);
    break;
  case kOpHypTangent:
    x = std::tanh(x);
    break;
  }

  table->Push(NodeValue::kFloat, x, this);
}

}
