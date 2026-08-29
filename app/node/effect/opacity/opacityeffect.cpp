/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "opacityeffect.h"

#include "node/math/math/math.h"
#include "widget/slider/floatslider.h"

namespace olive
{

#define super Node

const QString OpacityEffect::kTextureInput = QStringLiteral("tex_in");
const QString OpacityEffect::kValueInput = QStringLiteral("opacity_in");

OpacityEffect::OpacityEffect()
{
	MathNode *math = new MathNode();
	math->setParent(this);

	math->SetOperation(MathNode::kOpMultiply);

	SetNodePositionInContext(math, QPointF(0, 0));

	AddInput(kTextureInput, NodeValue::kTexture,
			 InputFlags(kInputFlagNotKeyframable));

	AddInput(kValueInput, NodeValue::kFloat, 1.0);
	SetInputProperty(kValueInput, QStringLiteral("view"),
					 FloatSlider::kPercentage);
	SetInputProperty(kValueInput, QStringLiteral("min"), 0.0);
	SetInputProperty(kValueInput, QStringLiteral("max"), 1.0);

	SetFlag(kVideoEffect);
	SetEffectInput(kTextureInput);
}

void OpacityEffect::Retranslate()
{
	super::Retranslate();

	SetInputName(kTextureInput, tr("Texture"));
	SetInputName(kValueInput, tr("Opacity"));
}

ShaderCode OpacityEffect::GetShaderCode(const ShaderRequest &request) const
{
	if (request.id == QStringLiteral("rgbmult")) {
		return ShaderCode(
			FileFunctions::ReadFileAsString(":/shaders/opacity_rgb.frag"));
	} else {
		return ShaderCode(
			FileFunctions::ReadFileAsString(":/shaders/opacity.frag"));
	}
}

void OpacityEffect::Value(const NodeValueRow &value, const NodeGlobals &globals,
						  NodeValueTable *table) const
{
	// If there's no texture, no need to run an operation
	if (TexturePtr tex = value[kTextureInput].toTexture()) {
		if (TexturePtr opacity_tex = value[kValueInput].toTexture()) {
			ShaderJob job(value);
			job.SetShaderID(QStringLiteral("rgbmult"));
			table->Push(NodeValue::kTexture, tex->toJob(job), this);
		} else if (!qFuzzyCompare(value[kValueInput].toDouble(), 1.0)) {
			table->Push(NodeValue::kTexture, tex->toJob(ShaderJob(value)),
						this);
		} else {
			// 1.0 float is a no-op, so just push the texture
			table->Push(value[kTextureInput]);
		}
	}
}

}
