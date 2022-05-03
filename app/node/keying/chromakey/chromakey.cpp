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

#include "chromakey.h"

#include "node/color/colormanager/colormanager.h"
#include "render/colorprocessor.h"

namespace olive {

#define super OCIOBaseNode

const QString ChromaKeyNode::kColorInput = QStringLiteral("color_key");
const QString ChromaKeyNode::kMaskOnlyInput = QStringLiteral("mask_only_in");
const QString ChromaKeyNode::kUpperToleranceInput = QStringLiteral("upper_tolerence_in");
const QString ChromaKeyNode::kLowerToleranceInput = QStringLiteral("lower_tolerence_in");

ChromaKeyNode::ChromaKeyNode()
{
  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(0.0f, 1.0f, 0.0f, 1.0f)));

  AddInput(kUpperToleranceInput, NodeValue::kFloat, 25.0);
  SetInputProperty(kUpperToleranceInput, QStringLiteral("min"), 0.0);

  AddInput(kLowerToleranceInput, NodeValue::kFloat, 5.0);
  SetInputProperty(kLowerToleranceInput, QStringLiteral("min"), 0.0);

  AddInput(kMaskOnlyInput, NodeValue::kBoolean, false);

  SetEffectInput(kTextureInput);
}

QString ChromaKeyNode::Name() const
{
  return tr("Chroma Key");
}

QString ChromaKeyNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.chromakey");
}

QVector<Node::CategoryID> ChromaKeyNode::Category() const
{
  return {kCategoryKeying};
}

QString ChromaKeyNode::Description() const
{
  return tr("A simple color key based on the distance from the chroma of a selected color.");
}

void ChromaKeyNode::Retranslate()
{
  super::Retranslate();
  SetInputName(kTextureInput, tr("Input"));
  //SetInputName(kGarbageMatteInput, tr("Garbage Matte"));
  //SetInputName(kCoreMatteInput, tr("Core Matte"));
  SetInputName(kColorInput, tr("Key Color"));
  //SetComboBoxStrings(kColorInput, {tr("Green"), tr("Blue")});
  //SetInputName(kShadowsInput, tr("Shadows"));
  //SetInputName(kHighlightsInput, tr("Highlights"));
  SetInputName(kUpperToleranceInput, tr("Upper Tolerance"));
  SetInputName(kLowerToleranceInput, tr("Lower Tolerance"));
  SetInputName(kMaskOnlyInput, tr("Show Mask Only"));
}

void ChromaKeyNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element);
  GenerateProcessor();
}

ShaderCode ChromaKeyNode::GetShaderCode(const QString &) const
{
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/chromakey.frag")));
}

void ChromaKeyNode::GenerateProcessor()
{
  if (manager()){
    ColorTransform transform("cie_xyz_d65_interchange");
    set_processor(ColorProcessor::Create(manager(), manager()->GetReferenceColorSpace(), transform));
  }
}

void ChromaKeyNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (!value[kTextureInput].data().isNull() && processor()) {
    ColorTransformJob job;

    job.InsertValue(value);
    job.SetAlphaChannelRequired(ColorTransformJob::kAlphaForceOn);
    job.SetColorProcessor(processor());
    job.SetInputTexture(value[kTextureInput].data().value<TexturePtr>());
    job.SetNeedsCustomShader(this);
    job.SetFunctionName(QStringLiteral("SceneLinearToCIEXYZ_d65"));

    table->Push(NodeValue::kColorTransformJob, QVariant::fromValue(job), this);
  }
}

} // namespace olive
