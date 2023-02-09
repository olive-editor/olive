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
const QString ChromaKeyNode::kInvertInput = QStringLiteral("invert_in");
const QString ChromaKeyNode::kUpperToleranceInput = QStringLiteral("upper_tolerence_in");
const QString ChromaKeyNode::kLowerToleranceInput = QStringLiteral("lower_tolerence_in");
const QString ChromaKeyNode::kGarbageMatteInput = QStringLiteral("garbage_in");
const QString ChromaKeyNode::kCoreMatteInput = QStringLiteral("core_in");
const QString ChromaKeyNode::kShadowsInput = QStringLiteral("shadows_in");
const QString ChromaKeyNode::kHighlightsInput = QStringLiteral("highlights_in");

ChromaKeyNode::ChromaKeyNode()
{
  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(0.0f, 1.0f, 0.0f, 1.0f)));

  AddInput(kLowerToleranceInput, NodeValue::kFloat, 5.0);
  SetInputProperty(kLowerToleranceInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kLowerToleranceInput, QStringLiteral("base"), 0.1);

  AddInput(kUpperToleranceInput, NodeValue::kFloat, 25.0);
  SetInputProperty(kUpperToleranceInput, QStringLiteral("base"), 0.1);

  // FIXME: Temporarily disabled. This will break if "lower tolerance" is keyframed or connected to
  //        something and there's currently no solution to remedy that. If there is in the future,
  //        we can look into re-enabling this.
  //SetInputProperty(kUpperToleranceInput, QStringLiteral("min"), GetStandardValue(kLowerToleranceInput).toDouble());

  AddInput(kGarbageMatteInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kCoreMatteInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kHighlightsInput, NodeValue::kFloat, 100.0f);
  SetInputProperty(kHighlightsInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kHighlightsInput, QStringLiteral("base"), 0.1);

  AddInput(kShadowsInput, NodeValue::kFloat, 100.0f);
  SetInputProperty(kShadowsInput, QStringLiteral("min"), 0.0);
  SetInputProperty(kShadowsInput, QStringLiteral("base"), 0.1);

  AddInput(kInvertInput, NodeValue::kBoolean, false);

  AddInput(kMaskOnlyInput, NodeValue::kBoolean, false);
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
  SetInputName(kGarbageMatteInput, tr("Garbage Matte"));
  SetInputName(kCoreMatteInput, tr("Core Matte"));
  SetInputName(kColorInput, tr("Key Color"));
  SetInputName(kShadowsInput, tr("Shadows"));
  SetInputName(kHighlightsInput, tr("Highlights"));
  SetInputName(kUpperToleranceInput, tr("Upper Tolerance"));
  SetInputName(kLowerToleranceInput, tr("Lower Tolerance"));
  SetInputName(kInvertInput, tr("Invert Mask"));
  SetInputName(kMaskOnlyInput, tr("Show Mask Only"));
}

void ChromaKeyNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element);
  if (input == kLowerToleranceInput) {
    // FIXME: Temporarily disabled. This will break if "lower tolerance" is keyframed or connected to
    //        something and there's currently no solution to remedy that. If there is in the future,
    //        we can look into re-enabling this.
    //SetInputProperty(kUpperToleranceInput, QStringLiteral("min"), GetStandardValue(kLowerToleranceInput).toDouble());
  }

  GenerateProcessor();
}

ShaderCode ChromaKeyNode::GetShaderCode(const ShaderRequest &request) const
{
  return ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/chromakey.frag")).arg(request.stub));
}

void ChromaKeyNode::GenerateProcessor()
{
  if (manager()){
    try {
      ColorTransform transform("cie_xyz_d65_interchange");
      set_processor(ColorProcessor::Create(manager(), manager()->GetReferenceColorSpace(), transform));
    } catch (const OCIO::Exception &e) {
      std::cerr << std::endl << e.what() << std::endl;
    }
  }
}

void ChromaKeyNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    if (processor()) {
      ColorTransformJob job(value);

      job.SetColorProcessor(processor());
      job.SetInputTexture(value[kTextureInput]);
      job.SetNeedsCustomShader(this);
      job.SetFunctionName(QStringLiteral("SceneLinearToCIEXYZ_d65"));

      table->Push(NodeValue::kTexture, tex->toJob(job), this);
    }
  }
}

void ChromaKeyNode::ConfigChanged()
{
  GenerateProcessor();
}

} // namespace olive
