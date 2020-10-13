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
#include "QVector3D"

OLIVE_NAMESPACE_ENTER

ColorFilterNode::ColorFilterNode()
{
  texture_input_ = new NodeInput("tex_in", NodeParam::kTexture);
  AddInput(texture_input_);

  offset_input_ = new NodeInput("offset_in", NodeParam::kColor, QColor(0.0, 0.0, 0.0, 1.0));
  AddInput(offset_input_);

  lift_input_ = new NodeInput("lift_in", NodeParam::kColor, QColor(0.0, 0.0, 0.0, 1.0));
  AddInput(lift_input_);

  gamma_input_ = new NodeInput("gamma_in", NodeParam::kColor, QColor(1.0, 1.0, 1.0, 1.0));
  AddInput(gamma_input_);

  gain_input_ = new NodeInput("gain_in", NodeParam::kColor, QColor(1.0, 1.0, 1.0, 1.0));
  AddInput(gain_input_);
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
  return tr("Color grading");
}

void ColorFilterNode::Retranslate()
{
  texture_input_->set_name(tr("Input"));
  offset_input_->set_name(tr("Offset"));
  lift_input_->set_name(tr("Lift"));
  gamma_input_->set_name(tr("Gamma"));
  gain_input_->set_name(tr("Gain"));
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
  job.InsertValue(lift_input_, value);
  job.InsertValue(gamma_input_, value);
  job.InsertValue(gain_input_, value);

  NodeValueTable table = value.Merge();

  // If there's no texture, no need to run an operation
  if (!job.GetValue(texture_input_).data().isNull()) {
    table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
  }

  return table;
}

OLIVE_NAMESPACE_EXIT
