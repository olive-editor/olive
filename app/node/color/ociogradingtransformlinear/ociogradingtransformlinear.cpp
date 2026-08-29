/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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
#include "node/project.h"
#include "render/colorprocessor.h"
#include "widget/slider/floatslider.h"

namespace olive
{

const QString OCIOGradingTransformLinearNode::kContrastInput =
	QStringLiteral("ocio_grading_primary_contrast");
const QString OCIOGradingTransformLinearNode::kOffsetInput =
	QStringLiteral("ocio_grading_primary_offset");
const QString OCIOGradingTransformLinearNode::kExposureInput =
	QStringLiteral("ocio_grading_primary_exposure");
const QString OCIOGradingTransformLinearNode::kSaturationInput =
	QStringLiteral("ocio_grading_primary_saturation");
const QString OCIOGradingTransformLinearNode::kPivotInput =
	QStringLiteral("ocio_grading_primary_pivot");
const QString OCIOGradingTransformLinearNode::kClampBlackEnableInput =
	QStringLiteral("clamp_black_enable_in");
const QString OCIOGradingTransformLinearNode::kClampBlackInput =
	QStringLiteral("ocio_grading_primary_clampBlack");
const QString OCIOGradingTransformLinearNode::kClampWhiteEnableInput =
	QStringLiteral("clamp_white_enable_in");
const QString OCIOGradingTransformLinearNode::kClampWhiteInput =
	QStringLiteral("ocio_grading_primary_clampWhite");

#define super OCIOBaseNode

OCIOGradingTransformLinearNode::OCIOGradingTransformLinearNode()
{
	AddInput(kContrastInput, NodeValue::kVec4, QVector4D{ 1.0, 1.0, 1.0, 1.0 });
	// Minimum based on OCIO::GradingPrimary::validate
	SetInputProperty(kContrastInput, QStringLiteral("min"),
					 QVector4D{ 0.01f, 0.01f, 0.01f, 0.01f });
	SetInputProperty(kContrastInput, QStringLiteral("base"), 0.01);
	SetVec4InputColors(kContrastInput);

	AddInput(kOffsetInput, NodeValue::kVec4, QVector4D{ 0.0, 0.0, 0.0, 0.0 });
	SetInputProperty(kOffsetInput, QStringLiteral("base"), 0.01);
	SetVec4InputColors(kOffsetInput);

	AddInput(kExposureInput, NodeValue::kVec4, QVector4D{ 0.0, 0.0, 0.0, 0.0 });
	SetInputProperty(kExposureInput, QStringLiteral("base"), 0.01);
	SetVec4InputColors(kExposureInput);

	AddInput(kSaturationInput, NodeValue::kFloat, 1.0);
	SetInputProperty(kSaturationInput, QStringLiteral("view"),
					 FloatSlider::kPercentage);
	SetInputProperty(kSaturationInput, QStringLiteral("min"), 0.0);

	AddInput(kPivotInput, NodeValue::kFloat,
			 0.18); // Default listed in OCIO::GradingPrimary
	SetInputProperty(kPivotInput, QStringLiteral("base"), 0.01);

	AddInput(kClampBlackEnableInput, NodeValue::kBoolean, false);

	AddInput(kClampBlackInput, NodeValue::kFloat, 0.0);
	SetInputProperty(kClampBlackInput, QStringLiteral("enabled"),
					 GetStandardValue(kClampBlackEnableInput).toBool());
	SetInputProperty(kClampBlackInput, QStringLiteral("base"), 0.01);

	AddInput(kClampWhiteEnableInput, NodeValue::kBoolean, false);

	AddInput(kClampWhiteInput, NodeValue::kFloat, 1.0);
	SetInputProperty(kClampWhiteInput, QStringLiteral("enabled"),
					 GetStandardValue(kClampWhiteEnableInput).toBool());
	SetInputProperty(kClampWhiteInput, QStringLiteral("base"), 0.01);

	// Constrain the white clamp minimum to just above the (static) black clamp
	// as per OCIO::GradingPrimary::validate. When the black clamp is keyframed
	// or connected, Value() enforces the invariant per frame instead.
	UpdateClampWhiteMinimum();
}

QString OCIOGradingTransformLinearNode::Name() const
{
	return tr("OCIO Color Grading (Linear)");
}

QString OCIOGradingTransformLinearNode::id() const
{
	return QStringLiteral(
		"org.olivevideoeditor.Olive.ociogradingtransformlinear");
}

QVector<Node::CategoryID> OCIOGradingTransformLinearNode::Category() const
{
	return { kCategoryColor };
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
	SetInputProperty(kExposureInput, QStringLiteral("tooltip"),
					 tr("Exposure increments in stops."));
	SetInputName(kSaturationInput, tr("Saturation"));
	SetInputName(kPivotInput, tr("Pivot"));
	SetInputName(kClampBlackEnableInput, tr("Enable Black Clamp"));
	SetInputName(kClampBlackInput, tr("Black Clamp"));
	SetInputName(kClampWhiteEnableInput, tr("Enable White Clamp"));
	SetInputName(kClampWhiteInput, tr("White Clamp"));
}

void OCIOGradingTransformLinearNode::InputValueChangedEvent(
	const QString &input, int element)
{
	Q_UNUSED(element);

	if (input == kClampWhiteEnableInput) {
		SetInputProperty(kClampWhiteInput, QStringLiteral("enabled"),
						 GetStandardValue(kClampWhiteEnableInput).toBool());
	} else if (input == kClampBlackEnableInput) {
		SetInputProperty(kClampBlackInput, QStringLiteral("enabled"),
						 GetStandardValue(kClampBlackEnableInput).toBool());
	} else if (input == kClampBlackInput) {
		// Ensure the white clamp is always greater than the black clamp as per
		// OCIO::GradingPrimary::validate
		UpdateClampWhiteMinimum();
	}

	GenerateProcessor();
}

void OCIOGradingTransformLinearNode::InputConnectedEvent(const QString &input,
														 int element, Node *output)
{
	super::InputConnectedEvent(input, element, output);

	if (input == kClampBlackInput) {
		UpdateClampWhiteMinimum();
	}
}

void OCIOGradingTransformLinearNode::InputDisconnectedEvent(const QString &input,
															int element,
															Node *output)
{
	super::InputDisconnectedEvent(input, element, output);

	if (input == kClampBlackInput) {
		UpdateClampWhiteMinimum();
	}
}

void OCIOGradingTransformLinearNode::UpdateClampWhiteMinimum()
{
	// A static UI minimum cannot follow an animated black clamp; for keyframed
	// or connected values the white>black invariant is enforced per frame in
	// Value() instead
	if (IsInputKeyframing(kClampBlackInput) ||
		IsInputConnected(kClampBlackInput)) {
		return;
	}

	SetInputProperty(kClampWhiteInput, QStringLiteral("min"),
					 GetStandardValue(kClampBlackInput).toDouble() + 0.000001);
}

void OCIOGradingTransformLinearNode::GenerateProcessor()
{
	if (manager()) {
		OCIO::GradingPrimaryTransformRcPtr gp =
			OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LIN);
		gp->makeDynamic();
		gp->setDirection(OCIO::TransformDirection::TRANSFORM_DIR_FORWARD);

		try {
			set_processor(ColorProcessor::Create(
				manager()->GetConfig()->getProcessor(gp)));
		} catch (const OCIO::Exception &e) {
			std::cerr << std::endl << e.what() << std::endl;
		}
	}
}

void OCIOGradingTransformLinearNode::Value(const NodeValueRow &value,
										   const NodeGlobals &globals,
										   NodeValueTable *table) const
{
	if (TexturePtr tex = value[kTextureInput].toTexture()) {
		if (processor()) {
			ColorTransformJob job(value);

			job.SetColorProcessor(processor());
			job.SetInputTexture(value[kTextureInput]);

			const int MASTER_CHANNEL = 0;
			const int RED_CHANNEL = 1;
			const int GREEN_CHANNEL = 2;
			const int BLUE_CHANNEL = 3;

			// Oddly, OCIO uses RGBMs when setting the GradingPrimary on the CPU, but uses vec3s on the GPU.
			// Even more oddly, the conversion from RGBM to vec3 does not appear to have a public API.
			// Therefore, this code has been duplicated from OCIO here:
			// https://github.com/AcademySoftwareFoundation/OpenColorIO/blob/3abbe5b20521169580fcfe3692aca81859859953/src/OpenColorIO/ops/gradingprimary/GradingPrimary.cpp#L157
			QVector4D offset = value[kOffsetInput].toVec4();
			offset[RED_CHANNEL] += offset[MASTER_CHANNEL];
			offset[GREEN_CHANNEL] += offset[MASTER_CHANNEL];
			offset[BLUE_CHANNEL] += offset[MASTER_CHANNEL];
			job.Insert(kOffsetInput,
					   NodeValue(NodeValue::kVec3,
								 QVector3D(offset[RED_CHANNEL],
										   offset[GREEN_CHANNEL],
										   offset[BLUE_CHANNEL])));

			QVector4D exposure = value[kExposureInput].toVec4();
			exposure[RED_CHANNEL] = std::pow(2.0f, exposure[MASTER_CHANNEL] +
													   exposure[RED_CHANNEL]);
			exposure[GREEN_CHANNEL] = std::pow(
				2.0f, exposure[MASTER_CHANNEL] + exposure[GREEN_CHANNEL]);
			exposure[BLUE_CHANNEL] = std::pow(2.0f, exposure[MASTER_CHANNEL] +
														exposure[BLUE_CHANNEL]);
			job.Insert(kExposureInput,
					   NodeValue(NodeValue::kVec3,
								 QVector3D(exposure[RED_CHANNEL],
										   exposure[GREEN_CHANNEL],
										   exposure[BLUE_CHANNEL])));

			QVector4D contrast = value[kContrastInput].toVec4();
			contrast[RED_CHANNEL] *= contrast[MASTER_CHANNEL];
			contrast[GREEN_CHANNEL] *= contrast[MASTER_CHANNEL];
			contrast[BLUE_CHANNEL] *= contrast[MASTER_CHANNEL];
			job.Insert(kContrastInput,
					   NodeValue(NodeValue::kVec3,
								 QVector3D(contrast[RED_CHANNEL],
										   contrast[GREEN_CHANNEL],
										   contrast[BLUE_CHANNEL])));

			if (!value[kClampBlackEnableInput].toBool()) {
				job.Insert(kClampBlackInput,
						   NodeValue(NodeValue::kFloat,
									 OCIO::GradingPrimary::NoClampBlack()));
			}

			if (!value[kClampWhiteEnableInput].toBool()) {
				job.Insert(kClampWhiteInput,
						   NodeValue(NodeValue::kFloat,
									 OCIO::GradingPrimary::NoClampWhite()));
			}

			if (value[kClampBlackEnableInput].toBool() &&
				value[kClampWhiteEnableInput].toBool()) {
				// OCIO::GradingPrimary::validate requires the white clamp to be
				// greater than the black clamp. Keyframed or connected values
				// can violate this at arbitrary times, so enforce the invariant
				// per frame here.
				const double clamp_black = value[kClampBlackInput].toDouble();
				const double clamp_white = value[kClampWhiteInput].toDouble();
				if (clamp_white <= clamp_black) {
					job.Insert(kClampWhiteInput,
							   NodeValue(NodeValue::kFloat,
										 clamp_black + 0.000001));
				}
			}

			table->Push(NodeValue::kTexture, tex->toJob(job), this);
		}
	}
}

void OCIOGradingTransformLinearNode::ConfigChanged()
{
	GenerateProcessor();
}

void OCIOGradingTransformLinearNode::SetVec4InputColors(const QString &input)
{
	SetInputProperty(input, QStringLiteral("color0"),
					 QColor(192, 192, 192).name());
	SetInputProperty(input, QStringLiteral("color1"), QColor(255, 0, 0).name());
	SetInputProperty(input, QStringLiteral("color2"), QColor(0, 255, 0).name());
	SetInputProperty(input, QStringLiteral("color3"), QColor(0, 0, 255).name());
}

}
