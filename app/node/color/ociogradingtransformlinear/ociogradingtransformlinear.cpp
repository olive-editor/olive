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

#include "ociogradingtransformlinear.h"

#include <iostream>

#include "common/ocioutils.h"
#include "node/project/project.h"
#include "render/colorprocessor.h"

namespace olive {

const QString OCIOGradingTransformLinearNode::kContrastInput = QStringLiteral("contrast_in");
const QString OCIOGradingTransformLinearNode::kOffsetInput = QStringLiteral("offset_in");
const QString OCIOGradingTransformLinearNode::kExposureInput = QStringLiteral("exposure_in");
const QString OCIOGradingTransformLinearNode::kSaturationInput = QStringLiteral("saturation_in");
const QString OCIOGradingTransformLinearNode::kPivotInput = QStringLiteral("pivot_in");
const QString OCIOGradingTransformLinearNode::kClampBlackInput = QStringLiteral("clamp_black_in");
const QString OCIOGradingTransformLinearNode::kClampWhiteInput = QStringLiteral("clamp_white_in");

#define super OCIOBaseNode

OCIOGradingTransformLinearNode::OCIOGradingTransformLinearNode()
{
  AddInput(kContrastInput, NodeValue::kVec4, QVector4D{1.0, 1.0, 1.0, 1.0});
  SetInputProperty(kContrastInput, QStringLiteral("min"), QVector4D{0.001f, 0.001f, 0.001f, 0.001f});
  SetVec4InputColors(kContrastInput);

  AddInput(kOffsetInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});
  SetVec4InputColors(kOffsetInput);

  AddInput(kExposureInput, NodeValue::kVec4, QVector4D{0.0, 0.0, 0.0, 0.0});
  SetVec4InputColors(kExposureInput);

  AddInput(kSaturationInput, NodeValue::kFloat, 1.0);
  SetInputProperty(kSaturationInput, QStringLiteral("min"), 0.0);

  AddInput(kPivotInput, NodeValue::kFloat, 0.203919098);

  AddInput(kClampBlackInput, NodeValue::kFloat, 0.0);

  AddInput(kClampWhiteInput, NodeValue::kFloat, 200.0);
}

QString OCIOGradingTransformLinearNode::Name() const
{
  return tr("OCIO Linear Grading Transform");
}

QString OCIOGradingTransformLinearNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.ociogradingtransformlinear");
}

QVector<Node::CategoryID> OCIOGradingTransformLinearNode::Category() const
{
  return {kCategoryColor};
}

QString OCIOGradingTransformLinearNode::Description() const
{
  return tr("Simple linear color grading using OpenColorIO.");
}

void OCIOGradingTransformLinearNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kContrastInput, tr("Contrast"));
  SetInputName(kOffsetInput, tr("Offset"));
  SetInputName(kExposureInput, tr("Exposure"));
  SetInputName(kSaturationInput, tr("Saturation"));
  SetInputName(kPivotInput, tr("Pivot"));
  SetInputName(kClampBlackInput, tr("Black Clamp"));
  SetInputName(kClampWhiteInput, tr("White Clamp"));
}

void OCIOGradingTransformLinearNode::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(element);
  GenerateProcessor();
}

void OCIOGradingTransformLinearNode::GenerateProcessor()
{
  OCIO::GradingPrimaryTransformRcPtr gp = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LIN);
  gp->setDirection(OCIO::TransformDirection::TRANSFORM_DIR_FORWARD);

  OCIO::GradingPrimary gpdata{OCIO::GRADING_LIN};
  gpdata.m_contrast = OCIOUtils::QVec4ToRGBM(GetStandardValue(kContrastInput).value<QVector4D>());
  gpdata.m_exposure = OCIOUtils::QVec4ToRGBM(GetStandardValue(kExposureInput).value<QVector4D>());
  gpdata.m_offset = OCIOUtils::QVec4ToRGBM(GetStandardValue(kOffsetInput).value<QVector4D>());
  gpdata.m_saturation = GetStandardValue(kSaturationInput).value<double>();
  gpdata.m_pivot = GetStandardValue(kPivotInput).value<double>();
  gpdata.m_clampBlack = GetStandardValue(kClampBlackInput).value<double>();
  gpdata.m_clampWhite = GetStandardValue(kClampWhiteInput).value<double>();
  try {
    gp->setValue(gpdata);

    set_processor(ColorProcessor::Create(manager()->GetConfig()->getProcessor(gp)));
  } catch (const OCIO::Exception &e) {
    std::cerr << std::endl << e.what() << std::endl;
  }
}

void OCIOGradingTransformLinearNode::ConfigChanged()
{
  GenerateProcessor();
}

void OCIOGradingTransformLinearNode::SetVec4InputColors(const QString &input)
{
  SetInputProperty(input, QStringLiteral("color0"), QColor(255, 0, 0).name());
  SetInputProperty(input, QStringLiteral("color1"), QColor(0, 255, 0).name());
  SetInputProperty(input, QStringLiteral("color2"), QColor(0, 0, 255).name());
  SetInputProperty(input, QStringLiteral("color3"), QColor(192, 192, 192).name());
}

}
