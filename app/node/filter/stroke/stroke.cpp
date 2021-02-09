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

const QString StrokeFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString StrokeFilterNode::kColorInput = QStringLiteral("color_in");
const QString StrokeFilterNode::kRadiusInput = QStringLiteral("radius_in");
const QString StrokeFilterNode::kOpacityInput = QStringLiteral("opacity_in");
const QString StrokeFilterNode::kInnerInput = QStringLiteral("inner_in");

StrokeFilterNode::StrokeFilterNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0f, 1.0f, 1.0f, 1.0f)));

  AddInput(kRadiusInput, NodeValue::kFloat, 10.0);
  SetInputProperty(kRadiusInput, QStringLiteral("min"), 0.0);

  AddInput(kOpacityInput, NodeValue::kFloat, 1.0f);
  SetInputProperty(kOpacityInput, QStringLiteral("view"), FloatSlider::kPercentage);
  SetInputProperty(kOpacityInput, QStringLiteral("min"), 0.0f);
  SetInputProperty(kOpacityInput, QStringLiteral("max"), 1.0f);

  AddInput(kInnerInput, NodeValue::kBoolean, false);
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
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kColorInput, tr("Color"));
  SetInputName(kRadiusInput, tr("Radius"));
  SetInputName(kOpacityInput, tr("Opacity"));
  SetInputName(kInnerInput, tr("Inner"));
}

NodeValueTable StrokeFilterNode::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  ShaderJob job;

  job.InsertValue(this, kTextureInput, value);
  job.InsertValue(this, kColorInput, value);
  job.InsertValue(this, kRadiusInput, value);
  job.InsertValue(this, kOpacityInput, value);
  job.InsertValue(this, kInnerInput, value);
  job.InsertValue(QStringLiteral("resolution_in"),
                  NodeValue(NodeValue::kVec2, value[QStringLiteral("global")].Get(NodeValue::kVec2, QStringLiteral("resolution")), this));

  NodeValueTable table = value.Merge();

  if (!job.GetValue(kTextureInput).data().isNull()) {
    if (job.GetValue(kRadiusInput).data().toDouble() > 0.0
        && job.GetValue(kOpacityInput).data().toDouble() > 0.0) {
      table.Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
    } else {
      table.Push(job.GetValue(kTextureInput));
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
