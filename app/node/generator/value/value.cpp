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

#include "value.h"


namespace olive {

ValueNode::ValueNode()
{
  type_input_ = new NodeInput("type_in", NodeParam::kCombo, 0);
  AddInput(type_input_);

  value_input_ = new NodeInput("value_in", NodeParam::kFloat, 1.0f);
  AddInput(value_input_);
}

Node* ValueNode::copy() const
{
  return new ValueNode();
}

QString ValueNode::Name() const
{
  return tr("Value");
}

QString ValueNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.value");
}

QVector<Node::CategoryID> ValueNode::Category() const
{
  return {kCategoryGenerator};
}

QString ValueNode::Description() const
{
  return tr("Generates a value");
}

void ValueNode::Retranslate()
{
  value_input_->set_name(tr("Value"));
  type_input_->set_combobox_strings({tr("Float"), tr("Integer")});
}

NodeValueTable ValueNode::Value(NodeValueDatabase &value) const
{
  NodeValueTable table = value.Merge();

  float x = value[value_input_].Take(NodeParam::kFloat).toFloat();

  table.Push(NodeParam::kFloat, x, this);

  return table;
}



}
