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

#include "dropshadowfilter.h"

#include "widget/slider/floatslider.h"

namespace olive {

#define super Node

const QString DropShadowFilter::kTextureInput = QStringLiteral("tex_in");
const QString DropShadowFilter::kColorInput = QStringLiteral("color_in");
const QString DropShadowFilter::kDistanceInput = QStringLiteral("distance_in");
const QString DropShadowFilter::kAngleInput = QStringLiteral("angle_in");
const QString DropShadowFilter::kSoftnessInput = QStringLiteral("radius_in");
const QString DropShadowFilter::kOpacityInput = QStringLiteral("opacity_in");
const QString DropShadowFilter::kFastInput = QStringLiteral("fast_in");

DropShadowFilter::DropShadowFilter()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(0.0, 0.0, 0.0)));

  AddInput(kDistanceInput, NodeValue::kFloat, 10.0);

  AddInput(kAngleInput, NodeValue::kFloat, 135.0);

  AddInput(kSoftnessInput, NodeValue::kFloat, 10.0);
  SetInputProperty(kSoftnessInput, QStringLiteral("min"), 0.0);

  AddInput(kOpacityInput, NodeValue::kFloat, 1.0);
  SetInputProperty(kOpacityInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kOpacityInput, QStringLiteral("view"), FloatSlider::kPercentage);

  AddInput(kFastInput, NodeValue::kBoolean, false);

  SetEffectInput(kTextureInput);
  SetFlag(kVideoEffect);
}

void DropShadowFilter::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Texture"));
  SetInputName(kColorInput, tr("Color"));
  SetInputName(kDistanceInput, tr("Distance"));
  SetInputName(kAngleInput, tr("Angle"));
  SetInputName(kSoftnessInput, tr("Softness"));
  SetInputName(kOpacityInput, tr("Opacity"));
  SetInputName(kFastInput, tr("Faster (Lower Quality)"));
}

ShaderCode DropShadowFilter::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/dropshadow.frag"));
}

void DropShadowFilter::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    ShaderJob job(value);

    QString iterative = QStringLiteral("previous_iteration_in");

    job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
    job.Insert(iterative, value[kTextureInput]);

    if (!qIsNull(value[kSoftnessInput].toDouble())) {
      job.SetIterations(3, iterative);
    }

    table->Push(NodeValue::kTexture, tex->toJob(job), this);
  }
}

}
