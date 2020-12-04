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

#include "stroke.h"

#include "render/color.h"

namespace olive {

StrokeFilterNode::StrokeFilterNode()
{
  tex_input_ = new NodeInput("tex_in", NodeParam::kTexture);
  AddInput(tex_input_);

  color_input_ = new NodeInput("color_in",
                               NodeParam::kColor,
                               QVariant::fromValue(Color(1.0f, 1.0f, 1.0f, 1.0f)));
  AddInput(color_input_);

  radius_input_ = new NodeInput("radius_in", NodeParam::kFloat, 10.0f);
  radius_input_->setProperty("min", 0.0f);
  AddInput(radius_input_);

  opacity_input_ = new NodeInput("opacity_in", NodeParam::kFloat, 1.0f);
  opacity_input_->setProperty("view", QStringLiteral("percent"));
  opacity_input_->setProperty("min", 0.0f);
  opacity_input_->setProperty("max", 1.0f);
  AddInput(opacity_input_);

  inner_input_ = new NodeInput("inner_in", NodeParam::kBoolean, false);
  AddInput(inner_input_);
}

Node *StrokeFilterNode::copy() const
{
  return new StrokeFilterNode();
}

QString StrokeFilterNode::Name() const
{
  return tr("Stroke");
}

QString StrokeFilterNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.stroke");
}

QVector<Node::CategoryID> StrokeFilterNode::Category() const
{
  return {kCategoryFilter};
}

QString StrokeFilterNode::Description() const
{
  return tr("Creates a stroke outline around an image.");
}

void StrokeFilterNode::Retranslate()
{
  tex_input_->set_name(tr("Input"));
  color_input_->set_name(tr("Color"));
  radius_input_->set_name(tr("Radius"));
  opacity_input_->set_name(tr("Opacity"));
  inner_input_->set_name(tr("Inner"));
}

NodeValueTable StrokeFilterNode::Value(NodeValueDatabase &value) const
{
  ShaderJob job;

  job.InsertValue(tex_input_, value);
  job.InsertValue(color_input_, value);
  job.InsertValue(radius_input_, value);
  job.InsertValue(opacity_input_, value);
  job.InsertValue(inner_input_, value);
  job.InsertValue(QStringLiteral("resolution_in"),
                  ShaderValue(value[QStringLiteral("global")].Get(NodeParam::kVec2, QStringLiteral("resolution")), NodeParam::kVec2));

  NodeValueTable table = value.Merge();

  if (!job.GetValue(tex_input_).data.isNull()) {
    if (job.GetValue(radius_input_).data.toDouble() > 0.0
        && job.GetValue(opacity_input_).data.toDouble() > 0.0) {
      table.Push(NodeParam::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table.Push(job.GetValue(tex_input_), this);
    }
  }

  return table;
}

ShaderCode StrokeFilterNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/stroke.frag"));
}

}
