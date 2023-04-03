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
  AddInput(kTextureInput, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kColorInput, TYPE_COLOR, Color(0.0, 0.0, 0.0));

  AddInput(kDistanceInput, TYPE_DOUBLE, 10.0);

  AddInput(kAngleInput, TYPE_DOUBLE, 135.0);

  AddInput(kSoftnessInput, TYPE_DOUBLE, 10.0);
  SetInputProperty(kSoftnessInput, QStringLiteral("min"), 0.0);

  AddInput(kOpacityInput, TYPE_DOUBLE, 1.0);
  SetInputProperty(kOpacityInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kOpacityInput, QStringLiteral("view"), FloatSlider::kPercentage);

  AddInput(kFastInput, TYPE_BOOL, false);

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

ShaderCode DropShadowFilter::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/dropshadow.frag"));
}

value_t DropShadowFilter::Value(const ValueParams &p) const
{
  value_t tex_meta = GetInputValue(p, kTextureInput);

  if (TexturePtr tex = tex_meta.toTexture()) {
    ShaderJob job = CreateShaderJob(p, GetShaderCode);

    QString iterative = QStringLiteral("previous_iteration_in");

    job.Insert(QStringLiteral("resolution_in"), tex->virtual_resolution());
    job.Insert(iterative, tex_meta);

    if (!qIsNull(GetInputValue(p, kSoftnessInput).toDouble())) {
      job.SetIterations(3, iterative);
    }

    return tex->toJob(job);
  }

  return value_t();
}

}
