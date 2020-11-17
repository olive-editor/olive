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

OLIVE_NAMESPACE_ENTER

MathNode::MathNode()
{
  method_in_ = new NodeInput(QStringLiteral("method_in"), NodeParam::kCombo);
  method_in_->set_connectable(false);
  method_in_->set_is_keyframable(false);
  AddInput(method_in_);

  param_a_in_ = new NodeInput(QStringLiteral("param_a_in"), NodeParam::kFloat, 0.0);
  param_a_in_->set_property(QStringLiteral("decimalplaces"), 8);
  param_a_in_->set_property(QStringLiteral("autotrim"), true);
  AddInput(param_a_in_);

  param_b_in_ = new NodeInput(QStringLiteral("param_b_in"), NodeParam::kFloat, 0.0);
  param_b_in_->set_property(QStringLiteral("decimalplaces"), 8);
  param_b_in_->set_property(QStringLiteral("autotrim"), true);
  AddInput(param_b_in_);
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

  method_in_->set_name(tr("Method"));
  param_a_in_->set_name(tr("Value"));
  param_b_in_->set_name(tr("Value"));

  QStringList operations = {tr("Add"),
                            tr("Subtract"),
                            tr("Multiply"),
                            tr("Divide"),
                            QString(),
                            tr("Power")};

  method_in_->set_combobox_strings(operations);
}

ShaderCode MathNode::GetShaderCode(const QString &shader_id) const
{
  return GetShaderCodeInternal(shader_id, param_a_in_, param_b_in_);
}

NodeValueTable MathNode::Value(NodeValueDatabase &value) const
{
  // Auto-detect what values to operate with
  // FIXME: Add manual override for this
  PairingCalculator calc(value[param_a_in_], value[param_b_in_]);

  // Do nothing if no pairing was found
  if (!calc.FoundMostLikelyPairing()) {
    return value.Merge();
  }

  NodeValue val_a = calc.GetMostLikelyValueA();
  value[param_a_in_].Remove(val_a);

  NodeValue val_b = calc.GetMostLikelyValueB();
  value[param_b_in_].Remove(val_b);

  return ValueInternal(value,
                       GetOperation(),
                       calc.GetMostLikelyPairing(),
                       param_a_in_,
                       val_a,
                       param_b_in_,
                       val_b);
}

void MathNode::ProcessSamples(NodeValueDatabase &values, const SampleBufferPtr input, SampleBufferPtr output, int index) const
{
  return ProcessSamplesInternal(values, GetOperation(), param_a_in_, param_b_in_, input, output, index);
}

OLIVE_NAMESPACE_EXIT
