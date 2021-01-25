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
#include "widget/slider/floatslider.h"

namespace olive {

StrokeFilterNode::StrokeFilterNode()
{
  tex_input_ = new NodeInput(this, QStringLiteral("tex_in"), NodeValue::kTexture);

  color_input_ = new NodeInput(this,
                               QStringLiteral("color_in"),
                               NodeValue::kColor,
                               QVariant::fromValue(Color(1.0f, 1.0f, 1.0f, 1.0f)));

  radius_input_ = new NodeInput(this, QStringLiteral("radius_in"), NodeValue::kFloat, 10.0f);
  radius_input_->setProperty("min", 0.0f);

  opacity_input_ = new NodeInput(this, QStringLiteral("opacity_in"), NodeValue::kFloat, 1.0f);
  opacity_input_->setProperty("view", FloatSlider::kPercentage);
  opacity_input_->setProperty("min", 0.0f);
  opacity_input_->setProperty("max", 1.0f);

  inner_input_ = new NodeInput(this, QStringLiteral("inner_in"), NodeValue::kBoolean, false);
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
                  NodeValue(NodeValue::kVec2, value[QStringLiteral("global")].Get(NodeValue::kVec2, QStringLiteral("resolution")), this));

  NodeValueTable table = value.Merge();

  if (!job.GetValue(tex_input_).data().isNull()) {
    if (job.GetValue(radius_input_).data().toDouble() > 0.0
        && job.GetValue(opacity_input_).data().toDouble() > 0.0) {
      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table.Push(job.GetValue(tex_input_));
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
