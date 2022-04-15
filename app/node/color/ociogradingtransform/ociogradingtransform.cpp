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

#define super OCIOBaseNode

OCIOGradingTransformNode::OCIOGradingTransformNode()
{
  AddInput(kTypeInput, NodeValue::kCombo, 0);

  //AddInput(kBrightnessInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});

  AddInput(kContrastInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0}, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
  SetInputProperty(kContrastInput, QStringLiteral("min"), QVector4D{0.001f, 0.001f, 0.001f, 0.001f});

  //AddInput(kGammaInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0});

  AddInput(kOffsetInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0}, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

  AddInput(kExposureInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0}, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

  //AddInput(kLiftInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});

  //AddInput(kGainInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0});

  AddInput(kSaturationInput, NodeValue::kFloat, 1.0, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
  SetInputProperty(kSaturationInput, QStringLiteral("min"), 0.0);

  AddInput(kPivotInput, NodeValue::kFloat, 0.203919098, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

 // AddInput(kPivotBlackInput, NodeValue::kFloat, 0.0);

  //AddInput(kPivotWhiteInput, NodeValue::kFloat, 1.0);

  AddInput(kClampBlackInput, NodeValue::kFloat, 0.0, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));

  AddInput(kClampWhiteInput, NodeValue::kFloat, 200.0, InputFlags(kInputFlagNotKeyframable | kInputFlagNotConnectable));
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
  super::Retranslate();

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
  ConfigChanged();
}

void OCIOGradingTransformNode::ConfigChanged()
{
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

    set_processor(ColorProcessor::Create(manager()->GetConfig()->getProcessor(gp)));
  } catch (const OCIO::Exception &e) {
    std::cerr << std::endl << e.what() << std::endl;
  }
}

}
