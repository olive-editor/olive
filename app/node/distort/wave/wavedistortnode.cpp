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

#include "wavedistortnode.h"

namespace olive {

const QString WaveDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString WaveDistortNode::kFrequencyInput = QStringLiteral("frequency_in");
const QString WaveDistortNode::kIntensityInput = QStringLiteral("intensity_in");
const QString WaveDistortNode::kEvolutionInput = QStringLiteral("evolution_in");
const QString WaveDistortNode::kVerticalInput = QStringLiteral("vertical_in");

#define super Node

WaveDistortNode::WaveDistortNode()
{
  AddInput(kTextureInput, TYPE_TEXTURE, kInputFlagNotKeyframable);

  AddInput(kFrequencyInput, TYPE_DOUBLE, 10.0);
  AddInput(kIntensityInput, TYPE_DOUBLE, 10.0);
  AddInput(kEvolutionInput, TYPE_DOUBLE, 0.0);

  AddInput(kVerticalInput, TYPE_COMBO, false);

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);
}

QString WaveDistortNode::Name() const
{
  return tr("Wave");
}

QString WaveDistortNode::id() const
{
  return QStringLiteral("org.oliveeditor.Olive.wave");
}

QVector<Node::CategoryID> WaveDistortNode::Category() const
{
  return {kCategoryDistort};
}

QString WaveDistortNode::Description() const
{
  return tr("Distorts an image along a sine wave.");
}

void WaveDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kFrequencyInput, tr("Frequency"));
  SetInputName(kIntensityInput, tr("Intensity"));
  SetInputName(kEvolutionInput, tr("Evolution"));
  SetInputName(kVerticalInput, tr("Direction"));
  SetComboBoxStrings(kVerticalInput, {tr("Horizontal"), tr("Vertical")});
}

ShaderCode WaveDistortNode::GetShaderCode(const QString &id)
{
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/wave.frag"));
}

value_t WaveDistortNode::Value(const ValueParams &p) const
{
  // If there's no texture, no need to run an operation
  value_t tex_meta = GetInputValue(p, kTextureInput);

  if (TexturePtr texture = tex_meta.toTexture()) {
    // Only run shader if at least one of flip or flop are selected
    if (!qIsNull(GetInputValue(p, kIntensityInput).toDouble())) {
      return Texture::Job(texture->params(), CreateShaderJob(p, GetShaderCode));
    }
  }

  return tex_meta;
}

}
