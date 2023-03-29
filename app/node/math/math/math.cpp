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

#include "math.h"

namespace olive {

const QString MathNode::kMethodIn = QStringLiteral("method_in");
const QString MathNode::kParamAIn = QStringLiteral("param_a_in");
const QString MathNode::kParamBIn = QStringLiteral("param_b_in");
const QString MathNode::kParamCIn = QStringLiteral("param_c_in");

#define super MathNodeBase

MathNode::MathNode()
{
  AddInput(kMethodIn, NodeValue::kCombo, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

  AddInput(kParamAIn, NodeValue::kFloat, 0.0);
  SetInputProperty(kParamAIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamAIn, QStringLiteral("autotrim"), true);

  AddInput(kParamBIn, NodeValue::kFloat, 0.0);
  SetInputProperty(kParamBIn, QStringLiteral("decimalplaces"), 8);
  SetInputProperty(kParamBIn, QStringLiteral("autotrim"), true);
}

QString MathNode::Name() const
{
  // Default to naming after the operation
  if (parent()) {
    QString op_name = GetOperationName(GetOperation());
    if (!op_name.isEmpty()) {
      return op_name;
    }
  }

  return tr("Math");
}

QString MathNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.math");
}

QVector<Node::CategoryID> MathNode::Category() const
{
  return {kCategoryMath};
}

QString MathNode::Description() const
{
  return tr("Perform a mathematical operation between two values.");
}

void MathNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kMethodIn, tr("Method"));
  SetInputName(kParamAIn, tr("Value"));
  SetInputName(kParamBIn, tr("Value"));

  QStringList operations = {GetOperationName(kOpAdd),
                            GetOperationName(kOpSubtract),
                            GetOperationName(kOpMultiply),
                            GetOperationName(kOpDivide),
                            QString(),
                            GetOperationName(kOpPower)};

  SetComboBoxStrings(kMethodIn, operations);
}

NodeValue MathNode::Value(const ValueParams &p) const
{
  // Auto-detect what values to operate with
  // FIXME: Very inefficient
  auto aval = GetInputValue(p, kParamAIn);
  auto bval = GetInputValue(p, kParamBIn);

  if (!bval.data().isValid()) {
    return aval;
  }
  if (!aval.data().isValid()) {
    return bval;
  }

  PairingCalculator calc(aval, bval);

  // Do nothing if no pairing was found
  if (calc.FoundMostLikelyPairing()) {
    return ValueInternal(GetOperation(),
                         calc.GetMostLikelyPairing(),
                         kParamAIn,
                         calc.GetMostLikelyValueA(),
                         kParamBIn,
                         calc.GetMostLikelyValueB(),
                         p);
  }

  return NodeValue();
}

}
