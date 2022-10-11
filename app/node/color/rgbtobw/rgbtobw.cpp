/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "rgbtobw.h"

#include "node/project/project.h"

namespace olive {

const QString RGBToBWNode::kTextureInput = QStringLiteral("tex_in");
const QString RGBToBWNode::kCustomWeightsEnableInput = QStringLiteral("custom_weights_enable_in");
const QString RGBToBWNode::kCustomWeightsInput = QStringLiteral("custom_weights_in");

#define super Node

RGBToBWNode::RGBToBWNode() :
    weights_edited_(false)
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kCustomWeightsEnableInput, NodeValue::kBoolean, false);

  AddInput(kCustomWeightsInput, NodeValue::kVec3, QVector3D{0.33f, 0.33f, 0.33f}); //TEMP

  SetInputProperty(kCustomWeightsInput, QStringLiteral("color0"), QColor(255, 0, 0).name());
  SetInputProperty(kCustomWeightsInput, QStringLiteral("color1"), QColor(0, 255, 0).name());
  SetInputProperty(kCustomWeightsInput, QStringLiteral("color2"), QColor(0, 0, 255).name());

  UpdateInputs(false);

  SetFlags(kVideoEffect);
  SetEffectInput(kTextureInput);
}

QString RGBToBWNode::Name() const
{
  return tr("RGB to B&&W");
}

QString RGBToBWNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.rgbtobw");
}

QVector<Node::CategoryID> RGBToBWNode::Category() const
{
  return {kCategoryColor};
}

QString RGBToBWNode::Description() const
{
  return tr("Converts an color image to a black and white image");
}

void RGBToBWNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kCustomWeightsEnableInput, tr("Use Custom Weights"));
  SetInputName(kCustomWeightsInput, tr("Weights"));

  // Only set custom weights to their default value if they haven't been edited yet
  if (!weights_edited_) {
    if (project()) {
      project()->color_manager()->GetDefaultLumaCoefs(luma_coeffs_);
      SetStandardValue(kCustomWeightsInput, QVector3D(luma_coeffs_[0], luma_coeffs_[1], luma_coeffs_[2]));
    }
  }
}

void RGBToBWNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element);

  if (input == kCustomWeightsEnableInput) {
    UpdateInputs(GetStandardValue(kCustomWeightsEnableInput).toBool());
  }
  if (input == kCustomWeightsInput) {
    weights_edited_ = true;
  }
 }
ShaderCode RGBToBWNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/rgbtobw.frag"));
}

void RGBToBWNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job(value);


  // Set luma coefficients
  double luma_coeffs[3] = {0.0f, 0.0f, 0.0f}; // Can this use the cached values?
  project()->color_manager()->GetDefaultLumaCoefs(luma_coeffs);
  job.Insert(QStringLiteral("luma_coeffs"),
                  NodeValue(NodeValue::kVec3, QVector3D(luma_coeffs[0], luma_coeffs[1], luma_coeffs[2])));

  // If there's no texture, no need to run an operation
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    table->Push(NodeValue::kTexture, tex->toJob(job), this);
  } else {
    table->Push(value[kTextureInput]);
  }
}

void RGBToBWNode::UpdateInputs(bool custom_coefficients)
{
  SetInputFlags(kCustomWeightsInput, custom_coefficients ? InputFlags() : InputFlags(kInputFlagHidden));
}

}

