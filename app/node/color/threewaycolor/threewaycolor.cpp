/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2026 mikesolar

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

#include "threewaycolor.h"

#include <QVector3D>

#include "node/project.h"
#include "widget/slider/floatslider.h"

namespace olive
{

#define super Node

const QString ThreeWayColorNode::kTextureInput = QStringLiteral("tex_in");
const QString ThreeWayColorNode::kShadowsColorInput =
	QStringLiteral("shadows_color_in");
const QString ThreeWayColorNode::kMidtonesColorInput =
	QStringLiteral("midtones_color_in");
const QString ThreeWayColorNode::kHighlightsColorInput =
	QStringLiteral("highlights_color_in");
const QString ThreeWayColorNode::kShadowsAmountInput =
	QStringLiteral("shadows_amount_in");
const QString ThreeWayColorNode::kMidtonesAmountInput =
	QStringLiteral("midtones_amount_in");
const QString ThreeWayColorNode::kHighlightsAmountInput =
	QStringLiteral("highlights_amount_in");
const QString ThreeWayColorNode::kLumaCoefficientsInput =
	QStringLiteral("luma_coefficients_in");

ThreeWayColorNode::ThreeWayColorNode()
{
	AddInput(kTextureInput, NodeValue::kTexture,
			 InputFlags(kInputFlagNotKeyframable));

	const QVariant neutral = QVariant::fromValue(Color(0.5, 0.5, 0.5, 1.0));
	AddInput(kShadowsColorInput, NodeValue::kColor, neutral);
	AddInput(kMidtonesColorInput, NodeValue::kColor, neutral);
	AddInput(kHighlightsColorInput, NodeValue::kColor, neutral);

	AddInput(kShadowsAmountInput, NodeValue::kFloat, 1.0);
	AddInput(kMidtonesAmountInput, NodeValue::kFloat, 1.0);
	AddInput(kHighlightsAmountInput, NodeValue::kFloat, 1.0);

	const QString min = QStringLiteral("min");
	const QString view = QStringLiteral("view");
	SetInputProperty(kShadowsAmountInput, min, 0.0);
	SetInputProperty(kMidtonesAmountInput, min, 0.0);
	SetInputProperty(kHighlightsAmountInput, min, 0.0);
	SetInputProperty(kShadowsAmountInput, view, FloatSlider::kPercentage);
	SetInputProperty(kMidtonesAmountInput, view, FloatSlider::kPercentage);
	SetInputProperty(kHighlightsAmountInput, view, FloatSlider::kPercentage);

	SetEffectInput(kTextureInput);
	SetFlag(kVideoEffect);
}

void ThreeWayColorNode::Retranslate()
{
	super::Retranslate();

	SetInputName(kTextureInput, tr("Input"));
	SetInputName(kShadowsColorInput, tr("Shadows"));
	SetInputName(kMidtonesColorInput, tr("Midtones"));
	SetInputName(kHighlightsColorInput, tr("Highlights"));
	SetInputName(kShadowsAmountInput, tr("Shadows Amount"));
	SetInputName(kMidtonesAmountInput, tr("Midtones Amount"));
	SetInputName(kHighlightsAmountInput, tr("Highlights Amount"));
}

ShaderCode ThreeWayColorNode::GetShaderCode(const ShaderRequest &request) const
{
	Q_UNUSED(request)
	return ShaderCode(
		FileFunctions::ReadFileAsString(":/shaders/threewaycolor.frag"));
}

void ThreeWayColorNode::Value(const NodeValueRow &value,
							  const NodeGlobals &globals,
							  NodeValueTable *table) const
{
	Q_UNUSED(globals)

	if (TexturePtr tex = value[kTextureInput].toTexture()) {
		ShaderJob job(value);

		double luma_coeffs[3] = { 0.0, 0.0, 0.0 };
		if (project() && project()->color_manager()) {
			project()->color_manager()->GetDefaultLumaCoefs(luma_coeffs);
		} else {
			luma_coeffs[0] = 0.2126;
			luma_coeffs[1] = 0.7152;
			luma_coeffs[2] = 0.0722;
		}
		job.Insert(kLumaCoefficientsInput,
				   NodeValue(NodeValue::kVec3,
							 QVector3D(luma_coeffs[0], luma_coeffs[1],
									   luma_coeffs[2])));

		table->Push(NodeValue::kTexture, tex->toJob(job), this);
	}
}

}
