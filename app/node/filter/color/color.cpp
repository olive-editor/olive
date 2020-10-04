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

#include "color.h"

OLIVE_NAMESPACE_ENTER

ColorFilterNode::ColorFilterNode()
{
  texture_input_ = new NodeInput("tex_in", NodeParam::kTexture);
  AddInput(texture_input_);

  offset_input_ = new NodeInput("offset_in", NodeParam::kFloat, false);
  AddInput(offset_input_);

  offset_r_input_ = new NodeInput("offset_r_in", NodeParam::kFloat, false);
  AddInput(offset_r_input_);

  offset_g_input_ = new NodeInput("offset_g_in", NodeParam::kFloat, false);
  AddInput(offset_g_input_);

  offset_b_input_ = new NodeInput("offset_b_in", NodeParam::kFloat, false);
  AddInput(offset_b_input_);
}

Node *ColorFilterNode::copy() const
{
  return new ColorFilterNode();
}

QString ColorFilterNode::Name() const
{
  return tr("Color");
}

QString ColorFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.colorgrade");
}

QList<Node::CategoryID> ColorFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString ColorFilterNode::Description() const
{
  return tr("Blurs an image.");
}

void ColorFilterNode::Retranslate()
{
  texture_input_->set_name(tr("Input"));
  offset_input_->set_name(tr("Offset"));
  offset_r_input_->set_name(tr("Offset Red"));
  offset_g_input_->set_name(tr("Offset Green"));
  offset_b_input_->set_name(tr("Offset Blue"));
}

ShaderCode ColorFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  return ShaderCode(ReadFileAsString(":/shaders/colorgrade.frag"), QString());
}

NodeValueTable ColorFilterNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(texture_input_, value);
  job.InsertValue(offset_input_, value);
  job.InsertValue(offset_r_input_, value);
  job.InsertValue(offset_g_input_, value);
  job.InsertValue(offset_b_input_, value);

  NodeValueTable table = value.Merge();

  // If there's no texture, no need to run an operation
  if (!job.GetValue(texture_input_).data().isNull()) {
    table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
  }

  return table;
}

OLIVE_NAMESPACE_EXIT
