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

#include "stroke.h"

#include "widget/slider/floatslider.h"

namespace olive {

const QString StrokeFilterNode::kTextureInput = QStringLiteral("tex_in");
const QString StrokeFilterNode::kColorInput = QStringLiteral("color_in");
const QString StrokeFilterNode::kRadiusInput = QStringLiteral("radius_in");
const QString StrokeFilterNode::kOpacityInput = QStringLiteral("opacity_in");
const QString StrokeFilterNode::kInnerInput = QStringLiteral("inner_in");

#define super Node

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

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
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
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kColorInput, tr("Color"));
  SetInputName(kRadiusInput, tr("Radius"));
  SetInputName(kOpacityInput, tr("Opacity"));
  SetInputName(kInnerInput, tr("Inner"));
}

NodeValue StrokeFilterNode::Value(const ValueParams &p) const
{
  NodeValue tex_meta = GetInputValue(p, kTextureInput);

  if (TexturePtr tex = tex_meta.toTexture()) {
    if (GetInputValue(p, kRadiusInput).toDouble() > 0.0
        && GetInputValue(p, kOpacityInput).toDouble() > 0.0) {
      ShaderJob job = CreateJob<ShaderJob>(p);
      job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
      return NodeValue(NodeValue::kTexture, tex->toJob(job), this);
    }
  }

  return tex_meta;
}

ShaderCode StrokeFilterNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)

  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/stroke.frag"));
}

}
