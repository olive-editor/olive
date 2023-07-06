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

#include "noise.h"

#include "widget/slider/floatslider.h"

namespace olive {

const QString NoiseGeneratorNode::kBaseIn = QStringLiteral("base_in");
const QString NoiseGeneratorNode::kColorInput = QStringLiteral("color_in");
const QString NoiseGeneratorNode::kStrengthInput = QStringLiteral("strength_in");

#define super Node

NoiseGeneratorNode::NoiseGeneratorNode()
{
  AddInput(kBaseIn, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kStrengthInput, TYPE_DOUBLE, 0.2);
  SetInputProperty(kStrengthInput, QStringLiteral("view"), FloatSlider::kPercentage);
  SetInputProperty(kStrengthInput, QStringLiteral("min"), 0.0);

  AddInput(kColorInput, TYPE_BOOL, false);

  SetEffectInput(kBaseIn);
  SetFlag(kVideoEffect);
}

QString NoiseGeneratorNode::Name() const
{
  return tr("Noise");
}

QString NoiseGeneratorNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.noise");
}

QVector<Node::CategoryID> NoiseGeneratorNode::Category() const
{
  return {kCategoryGenerator};
}

QString NoiseGeneratorNode::Description() const
{
  return tr("Generates noise patterns");
}

void NoiseGeneratorNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kBaseIn, tr("Base"));
  SetInputName(kStrengthInput, tr("Strength"));
  SetInputName(kColorInput, tr("Color"));
}

ShaderCode NoiseGeneratorNode::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/noise.frag"));
}

value_t NoiseGeneratorNode::Value(const ValueParams &p) const
{
  ShaderJob job = CreateShaderJob(p, GetShaderCode);

  job.Insert(QStringLiteral("time_in"), p.time().in().toDouble());

  TexturePtr base = GetInputValue(p, kBaseIn).toTexture();

  return Texture::Job(base ? base->params() : p.vparams(), job);
}
}
