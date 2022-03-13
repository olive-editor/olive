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

#include "ociogradingtransform.h"

#include "common/ocioutils.h"
#include "node/project/project.h"
#include "render/colorprocessor.h"

namespace olive {

const QString OCIOGradingTransformNode::kTextureInput = QStringLiteral("tex_in");
const QString OCIOGradingTransformNode::kTypeInput = QStringLiteral("type_in");
const QString OCIOGradingTransformNode::kBrightnessInput = QStringLiteral("brightness_in");
const QString OCIOGradingTransformNode::kContrastInput = QStringLiteral("contrast_in");
const QString OCIOGradingTransformNode::kGammaInput = QStringLiteral("gamma_in");
const QString OCIOGradingTransformNode::kOffsetInput = QStringLiteral("offset_in");
const QString OCIOGradingTransformNode::kExposureInput = QStringLiteral("exposure_in");
const QString OCIOGradingTransformNode::kLiftInput = QStringLiteral("lift_in");
const QString OCIOGradingTransformNode::kGainInput = QStringLiteral("gain_in");
const QString OCIOGradingTransformNode::kSaturationInput = QStringLiteral("saturation_in");
const QString OCIOGradingTransformNode::kPivotInput = QStringLiteral("pivot_in");
const QString OCIOGradingTransformNode::kPivotBlackInput = QStringLiteral("pivot_black_in");
const QString OCIOGradingTransformNode::kPivotWhiteInput = QStringLiteral("pivot_white_in");
const QString OCIOGradingTransformNode::kClampBlackInput = QStringLiteral("clamp_black_in");
const QString OCIOGradingTransformNode::kClampWhiteInput = QStringLiteral("clamp_white_in");

OCIOGradingTransformNode::OCIOGradingTransformNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kTypeInput, NodeValue::kCombo, 0);

  //AddInput(kBrightnessInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});

  AddInput(kContrastInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0});
  SetInputProperty(kContrastInput, QStringLiteral("min"), QVector4D{0.001f, 0.001f, 0.001f, 0.001f});

  //AddInput(kGammaInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0});

  AddInput(kOffsetInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});

  AddInput(kExposureInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});

  //AddInput(kLiftInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});

  //AddInput(kGainInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0});

  AddInput(kSaturationInput, NodeValue::kFloat, 1.0);
  SetInputProperty(kSaturationInput, QStringLiteral("min"), 0.0);

  AddInput(kPivotInput, NodeValue::kFloat, 0.203919098);

 // AddInput(kPivotBlackInput, NodeValue::kFloat, 0.0);

  //AddInput(kPivotWhiteInput, NodeValue::kFloat, 1.0);

  AddInput(kClampBlackInput, NodeValue::kFloat, 0.0);

  AddInput(kClampWhiteInput, NodeValue::kFloat, 200.0);

  GenerateProcessor();
}

void OCIOGradingTransformNode::GenerateProcessor()
{
  if (!project()) {
    return;
  }

  // Create shader description
  shader_desc_ = OCIO::GpuShaderDesc::CreateShaderDesc();
  shader_desc_->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
  shader_desc_->setFunctionName("OCIOGradingTransform");
  shader_desc_->setResourcePrefix("ocio_");

  OCIO::GradingPrimaryTransformRcPtr gp = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LIN);
  //auto gp = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LIN);
  gp->setDirection(OCIO::TransformDirection::TRANSFORM_DIR_FORWARD);
  //gp->makeDynamic();

  OCIO::GradingPrimary gpdata{OCIO::GRADING_LIN};
  //gpdata.m_lift = OCIOUtils::QVec4ToRGBM(GetStandardValue(kLiftInput).value<QVector4D>());
  gpdata.m_contrast = OCIOUtils::QVec4ToRGBM(GetStandardValue(kContrastInput).value<QVector4D>());
  gpdata.m_exposure = OCIOUtils::QVec4ToRGBM(GetStandardValue(kExposureInput).value<QVector4D>());
  //gpdata.m_gamma = OCIOUtils::QVec4ToRGBM(GetStandardValue(kGammaInput).value<QVector4D>());
  //gpdata.m_gain = OCIOUtils::QVec4ToRGBM(GetStandardValue(kGainInput).value<QVector4D>());
  gpdata.m_offset = OCIOUtils::QVec4ToRGBM(GetStandardValue(kOffsetInput).value<QVector4D>());
  gpdata.m_saturation = GetStandardValue(kSaturationInput).value<double>();
  gpdata.m_pivot = GetStandardValue(kPivotInput).value<double>();
  gpdata.m_clampBlack = GetStandardValue(kClampBlackInput).value<double>();
  gpdata.m_clampWhite = GetStandardValue(kClampWhiteInput).value<double>();
  //gpdata.m_pivotBlack = GetStandardValue(kPivotBlackInput).value<double>();
  //gpdata.m_pivotWhite = GetStandardValue(kPivotWhiteInput).value<double>();
  try {
    gp->setValue(gpdata);

    processor_ = std::make_shared<ColorProcessor>();
    processor_->SetProsessor(project()->color_manager()->GetConfig()->getProcessor(gp));
  } catch (const OCIO::Exception &e) {
    std::cerr << std::endl << e.what() << std::endl;
  }

  processor_->GetProcessor()->getDefaultGPUProcessor()->extractGpuShaderInfo(shader_desc_);
}

Node *OCIOGradingTransformNode::copy() const
{
  return new OCIOGradingTransformNode();
}

QString OCIOGradingTransformNode::Name() const
{
  return tr("OCIO Grading Transform");
}

QString OCIOGradingTransformNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.ociogradingtransform");
}

QVector<Node::CategoryID> OCIOGradingTransformNode::Category() const
{
  return {kCategoryColor};
}

QString OCIOGradingTransformNode::Description() const
{
  return tr("Simple color grading using OCIO");
}

void OCIOGradingTransformNode::Retranslate()
{
  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kTypeInput, tr("Type"));
  //SetInputName(kBrightnessInput, tr("Brightness"));
  SetInputName(kContrastInput, tr("Contrast"));
  //SetInputName(kGammaInput, tr("Gamma"));
  SetInputName(kOffsetInput, tr("Offset"));
  SetInputName(kExposureInput, tr("Exposure"));
  //SetInputName(kLiftInput, tr("Lift"));
  //SetInputName(kGainInput, tr("Gain"));
  SetInputName(kSaturationInput, tr("Saturation"));
  SetInputName(kPivotInput, tr("Pivot"));
  //SetInputName(kPivotBlackInput, tr("Black Pivot"));
  //SetInputName(kPivotWhiteInput, tr("White Pivot"));
  SetInputName(kClampBlackInput, tr("Black Clamp"));
  SetInputName(kClampWhiteInput, tr("White Clamp"));

  SetComboBoxStrings(kTypeInput, {"Linear"});
}

void OCIOGradingTransformNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element);
  GenerateProcessor();
}

ShaderCode OCIOGradingTransformNode::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)
  // Generate shader code using OCIO stub and our auto-generated name
  QString shader_frag = FileFunctions::ReadFileAsString(QStringLiteral(":shaders/ociogradingtransform.frag"))
                            .arg(shader_desc_->getShaderText());
  return ShaderCode(shader_frag);
}

void OCIOGradingTransformNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  ShaderJob job;
  job.InsertValue(value);
  job.SetUseOCIO(true);
  job.SetShaderDesc(shader_desc_);
  job.SetColorProcessor(processor_);

  // If there's no texture, no need to run an operation
  if (!job.GetValue(kTextureInput).data().isNull()) {
    table->Push(NodeValue::kShaderJob, QVariant::fromValue(job), this);
  }
}

}
