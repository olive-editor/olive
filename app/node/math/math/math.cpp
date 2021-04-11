/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

Node *MathNode::copy() const
{
  return new MathNode();
}

QString MathNode::Name() const
{
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
  Node::Retranslate();

  SetInputName(kMethodIn, tr("Method"));
  SetInputName(kParamAIn, tr("Value"));
  SetInputName(kParamBIn, tr("Value"));

  QStringList operations = {tr("Add"),
                            tr("Subtract"),
                            tr("Multiply"),
                            tr("Divide"),
                            QString(),
                            tr("Power")};

  SetComboBoxStrings(kMethodIn, operations);
}

ShaderCode MathNode::GetShaderCode(const QString &shader_id) const
{
  return GetShaderCodeInternal(shader_id, kParamAIn, kParamBIn);
}

NodeValueTable MathNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  // Auto-detect what values to operate with
  // FIXME: Add manual override for this
  PairingCalculator calc(value[kParamAIn], value[kParamBIn]);

  // Do nothing if no pairing was found
  if (!calc.FoundMostLikelyPairing()) {
    return value.Merge();
  }

  NodeValue val_a = calc.GetMostLikelyValueA();
  value[kParamAIn].Remove(val_a);

  NodeValue val_b = calc.GetMostLikelyValueB();
  value[kParamBIn].Remove(val_b);

  return ValueInternal(value,
                       GetOperation(),
                       calc.GetMostLikelyPairing(),
                       kParamAIn,
                       val_a,
                       kParamBIn,
                       val_b);
}

void MathNode::ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  return ProcessSamplesInternal(values, GetOperation(), kParamAIn, kParamBIn, input, output, index);
}

}
