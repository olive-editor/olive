#include <gtest/gtest.h>

#include <QPointF>
#include <QStringList>
#include <QVector2D>
#include <QVector3D>

#include "node/color/colormanager/colormanager.h"
#include "node/effect/opacity/opacityeffect.h"
#include "node/filter/blur/blur.h"
#include "node/filter/dropshadow/dropshadowfilter.h"
#include "node/filter/mosaic/mosaicfilternode.h"
#include "node/filter/stroke/stroke.h"
#include "node/keying/chromakey/chromakey.h"
#include "node/keying/colordifferencekey/colordifferencekey.h"
#include "node/keying/despill/despill.h"
#include "node/project.h"
#include "olive/core/util/color.h"
#include "render/job/colortransformjob.h"
#include "render/job/shaderjob.h"
#include "render/texture.h"
#include "widget/slider/floatslider.h"

namespace
{

// A "dummy" texture has no renderer backend and is therefore safe to pass
// around in a headless, CPU-only test.
olive::TexturePtr MakeDummyTexture()
{
	return std::make_shared<olive::Texture>(
		olive::VideoParams(16, 16, olive::core::PixelFormat::F32,
						   olive::VideoParams::kRGBAChannelCount));
}

olive::NodeValue TextureValue(const olive::TexturePtr &tex)
{
	return olive::NodeValue(olive::NodeValue::kTexture, tex);
}

olive::NodeValue FloatValue(double d)
{
	return olive::NodeValue(olive::NodeValue::kFloat, d);
}

olive::NodeValue BoolValue(bool b)
{
	return olive::NodeValue(olive::NodeValue::kBoolean, b);
}

olive::NodeValue ComboValue(int i)
{
	return olive::NodeValue(olive::NodeValue::kCombo, i);
}

olive::NodeValue Vec2Value(const QVector2D &v)
{
	return olive::NodeValue(olive::NodeValue::kVec2, v);
}

olive::NodeValueRow MakeTextureRow(const QString &input,
								   const olive::TexturePtr &tex)
{
	olive::NodeValueRow row;
	row.insert(input, TextureValue(tex));
	return row;
}

} // namespace

// -----------------------------------------------------------------------------
// OpacityEffect
// -----------------------------------------------------------------------------

TEST(OpacityEffect, InputDefinitionsAndDefaults)
{
	olive::OpacityEffect node;

	EXPECT_TRUE(node.HasInputWithID(olive::OpacityEffect::kTextureInput));
	EXPECT_TRUE(node.HasInputWithID(olive::OpacityEffect::kValueInput));

	EXPECT_EQ(int(node.GetInputDataType(olive::OpacityEffect::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_EQ(int(node.GetInputDataType(olive::OpacityEffect::kValueInput)),
			  int(olive::NodeValue::kFloat));

	// The texture input is a static effect input: not keyframable.
	EXPECT_FALSE(node.IsInputKeyframable(olive::OpacityEffect::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::OpacityEffect::kTextureInput);

	// Opacity is a 0-100% slider defaulting to fully opaque.
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::OpacityEffect::kValueInput).toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::OpacityEffect::kValueInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::OpacityEffect::kValueInput,
										   QStringLiteral("max"))
						 .toDouble(),
					 1.0);
	EXPECT_EQ(node.GetInputProperty(olive::OpacityEffect::kValueInput,
									QStringLiteral("view"))
				  .toInt(),
			  int(olive::FloatSlider::kPercentage));
}

TEST(OpacityEffect, Identity)
{
	olive::OpacityEffect node;

	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.opacity"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryFilter));
}

TEST(OpacityEffect, RetranslateSetsInputNames)
{
	olive::OpacityEffect node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::OpacityEffect::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::OpacityEffect::kValueInput),
			  QStringLiteral("Opacity"));
}

TEST(OpacityEffect, ShaderCodeSelectsFragmentById)
{
	olive::OpacityEffect node;

	const olive::ShaderCode mult =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("rgbmult")));
	EXPECT_FALSE(mult.frag_code().isEmpty());
	EXPECT_TRUE(mult.vert_code().isEmpty());

	const olive::ShaderCode plain =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("other")));
	EXPECT_FALSE(plain.frag_code().isEmpty());
	EXPECT_TRUE(plain.vert_code().isEmpty());

	// The two requests resolve to different fragment shaders.
	EXPECT_NE(mult.frag_code(), plain.frag_code());
}

TEST(OpacityEffect, ValueWithoutTexturePushesNothing)
{
	olive::OpacityEffect node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(OpacityEffect, ValueWithFullOpacityPassesTextureThrough)
{
	olive::OpacityEffect node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OpacityEffect::kTextureInput, tex);
	row.insert(olive::OpacityEffect::kValueInput, FloatValue(1.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	// 1.0 is a no-op: the input texture is pushed unchanged, not a job.
	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(OpacityEffect, ValueWithFractionalOpacityPushesShaderJob)
{
	olive::OpacityEffect node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OpacityEffect::kTextureInput, tex);
	row.insert(olive::OpacityEffect::kValueInput, FloatValue(0.5));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// The default shader (no special ID) is used for a plain float multiply.
	EXPECT_TRUE(job->GetShaderID().isEmpty());

	const olive::NodeValueRow &values = job->GetValues();
	ASSERT_TRUE(values.contains(olive::OpacityEffect::kValueInput));
	EXPECT_DOUBLE_EQ(values.value(olive::OpacityEffect::kValueInput).toDouble(),
					 0.5);
	EXPECT_EQ(values.value(olive::OpacityEffect::kTextureInput).toTexture(),
			  tex);
}

TEST(OpacityEffect, ValueWithTextureOpacityPushesRgbMultJob)
{
	olive::OpacityEffect node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::TexturePtr opacity_tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OpacityEffect::kTextureInput, tex);
	row.insert(olive::OpacityEffect::kValueInput, TextureValue(opacity_tex));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// A texture-valued opacity selects the rgbmult shader.
	EXPECT_EQ(job->GetShaderID(), QStringLiteral("rgbmult"));
	EXPECT_EQ(job->GetValues()
				  .value(olive::OpacityEffect::kValueInput)
				  .toTexture(),
			  opacity_tex);
}

// -----------------------------------------------------------------------------
// BlurFilterNode
// -----------------------------------------------------------------------------

TEST(BlurFilterNode, InputDefinitionsAndDefaults)
{
	olive::BlurFilterNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::BlurFilterNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::BlurFilterNode::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::BlurFilterNode::kTextureInput);

	// Method is a static UI choice defaulting to Gaussian.
	EXPECT_EQ(int(node.GetInputDataType(olive::BlurFilterNode::kMethodInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_FALSE(node.IsInputKeyframable(olive::BlurFilterNode::kMethodInput));
	EXPECT_FALSE(node.IsInputConnectable(olive::BlurFilterNode::kMethodInput));
	EXPECT_EQ(node.GetStandardValue(olive::BlurFilterNode::kMethodInput).toInt(),
			  int(olive::BlurFilterNode::kGaussian));
	EXPECT_EQ(int(node.GetMethod()), int(olive::BlurFilterNode::kGaussian));

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::BlurFilterNode::kRadiusInput).toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::BlurFilterNode::kRadiusInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);

	EXPECT_TRUE(
		node.GetStandardValue(olive::BlurFilterNode::kHorizInput).toBool());
	EXPECT_TRUE(
		node.GetStandardValue(olive::BlurFilterNode::kVertInput).toBool());
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::BlurFilterNode::kDirectionalDegreesInput)
			.toDouble(),
		0.0);
	EXPECT_EQ(node.GetStandardValue(olive::BlurFilterNode::kRadialCenterInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));
	EXPECT_TRUE(node.GetStandardValue(
					 olive::BlurFilterNode::kRepeatEdgePixelsInput)
					.toBool());
}

TEST(BlurFilterNode, Identity)
{
	olive::BlurFilterNode node;

	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.blur"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryFilter));
}

TEST(BlurFilterNode, RetranslateSetsInputNamesAndComboStrings)
{
	olive::BlurFilterNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::BlurFilterNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::BlurFilterNode::kMethodInput),
			  QStringLiteral("Method"));
	EXPECT_EQ(node.GetComboBoxStrings(olive::BlurFilterNode::kMethodInput),
			  QStringList({ QStringLiteral("Box"), QStringLiteral("Gaussian"),
							QStringLiteral("Directional"),
							QStringLiteral("Radial") }));
	EXPECT_EQ(node.GetInputName(olive::BlurFilterNode::kRadiusInput),
			  QStringLiteral("Radius"));
	EXPECT_EQ(node.GetInputName(olive::BlurFilterNode::kHorizInput),
			  QStringLiteral("Horizontal"));
	EXPECT_EQ(node.GetInputName(olive::BlurFilterNode::kVertInput),
			  QStringLiteral("Vertical"));
	EXPECT_EQ(
		node.GetInputName(olive::BlurFilterNode::kRepeatEdgePixelsInput),
		QStringLiteral("Repeat Edge Pixels"));
	EXPECT_EQ(
		node.GetInputName(olive::BlurFilterNode::kDirectionalDegreesInput),
		QStringLiteral("Direction"));
	EXPECT_EQ(node.GetInputName(olive::BlurFilterNode::kRadialCenterInput),
			  QStringLiteral("Center"));
}

TEST(BlurFilterNode, MethodSwitchTogglesInputVisibility)
{
	olive::BlurFilterNode node;

	// Default method (Gaussian) shows the axis toggles only.
	EXPECT_FALSE(node.IsInputHidden(olive::BlurFilterNode::kHorizInput));
	EXPECT_FALSE(node.IsInputHidden(olive::BlurFilterNode::kVertInput));
	EXPECT_TRUE(
		node.IsInputHidden(olive::BlurFilterNode::kDirectionalDegreesInput));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kRadialCenterInput));

	node.SetStandardValue(olive::BlurFilterNode::kMethodInput,
						  int(olive::BlurFilterNode::kDirectional));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kHorizInput));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kVertInput));
	EXPECT_FALSE(
		node.IsInputHidden(olive::BlurFilterNode::kDirectionalDegreesInput));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kRadialCenterInput));

	node.SetStandardValue(olive::BlurFilterNode::kMethodInput,
						  int(olive::BlurFilterNode::kRadial));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kHorizInput));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kVertInput));
	EXPECT_TRUE(
		node.IsInputHidden(olive::BlurFilterNode::kDirectionalDegreesInput));
	EXPECT_FALSE(node.IsInputHidden(olive::BlurFilterNode::kRadialCenterInput));

	node.SetStandardValue(olive::BlurFilterNode::kMethodInput,
						  int(olive::BlurFilterNode::kBox));
	EXPECT_FALSE(node.IsInputHidden(olive::BlurFilterNode::kHorizInput));
	EXPECT_FALSE(node.IsInputHidden(olive::BlurFilterNode::kVertInput));
	EXPECT_TRUE(
		node.IsInputHidden(olive::BlurFilterNode::kDirectionalDegreesInput));
	EXPECT_TRUE(node.IsInputHidden(olive::BlurFilterNode::kRadialCenterInput));
}

TEST(BlurFilterNode, ShaderCodeLoadsFragmentResource)
{
	olive::BlurFilterNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(BlurFilterNode, ValueWithoutTexturePushesNothing)
{
	olive::BlurFilterNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(BlurFilterNode, ValueWithZeroRadiusPassesTextureThrough)
{
	olive::BlurFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::BlurFilterNode::kTextureInput, tex);
	row.insert(olive::BlurFilterNode::kMethodInput,
			   ComboValue(int(olive::BlurFilterNode::kGaussian)));
	row.insert(olive::BlurFilterNode::kRadiusInput, FloatValue(0.0));
	row.insert(olive::BlurFilterNode::kHorizInput, BoolValue(true));
	row.insert(olive::BlurFilterNode::kVertInput, BoolValue(true));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	// No radius means no blur: the texture passes through unchanged.
	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(BlurFilterNode, ValueWithBothAxesPushesTwoIterationJob)
{
	olive::BlurFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::BlurFilterNode::kTextureInput, tex);
	row.insert(olive::BlurFilterNode::kMethodInput,
			   ComboValue(int(olive::BlurFilterNode::kGaussian)));
	row.insert(olive::BlurFilterNode::kRadiusInput, FloatValue(10.0));
	row.insert(olive::BlurFilterNode::kHorizInput, BoolValue(true));
	row.insert(olive::BlurFilterNode::kVertInput, BoolValue(true));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// Blurring both axes runs the shader twice, feeding the texture input.
	EXPECT_EQ(job->GetIterationCount(), 2);
	EXPECT_EQ(job->GetIterativeInput(), olive::BlurFilterNode::kTextureInput);

	const olive::NodeValueRow &values = job->GetValues();
	ASSERT_TRUE(values.contains(QStringLiteral("resolution_in")));
	EXPECT_EQ(values.value(QStringLiteral("resolution_in")).toVec2(),
			  QVector2D(16.0f, 16.0f));
	EXPECT_DOUBLE_EQ(
		values.value(olive::BlurFilterNode::kRadiusInput).toDouble(), 10.0);
}

TEST(BlurFilterNode, ValueWithSingleAxisPushesOneIterationJob)
{
	olive::BlurFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::BlurFilterNode::kTextureInput, tex);
	row.insert(olive::BlurFilterNode::kMethodInput,
			   ComboValue(int(olive::BlurFilterNode::kGaussian)));
	row.insert(olive::BlurFilterNode::kRadiusInput, FloatValue(10.0));
	row.insert(olive::BlurFilterNode::kHorizInput, BoolValue(true));
	row.insert(olive::BlurFilterNode::kVertInput, BoolValue(false));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);
	EXPECT_EQ(job->GetIterationCount(), 1);
	EXPECT_EQ(job->GetIterativeInput(), olive::BlurFilterNode::kTextureInput);
}

TEST(BlurFilterNode, ValueWithNoAxesPassesTextureThrough)
{
	olive::BlurFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::BlurFilterNode::kTextureInput, tex);
	row.insert(olive::BlurFilterNode::kMethodInput,
			   ComboValue(int(olive::BlurFilterNode::kGaussian)));
	row.insert(olive::BlurFilterNode::kRadiusInput, FloatValue(10.0));
	row.insert(olive::BlurFilterNode::kHorizInput, BoolValue(false));
	row.insert(olive::BlurFilterNode::kVertInput, BoolValue(false));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	// Both axes unchecked disables the blur entirely.
	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(BlurFilterNode, ValueWithDirectionalMethodPushesJob)
{
	olive::BlurFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::BlurFilterNode::kTextureInput, tex);
	row.insert(olive::BlurFilterNode::kMethodInput,
			   ComboValue(int(olive::BlurFilterNode::kDirectional)));
	row.insert(olive::BlurFilterNode::kRadiusInput, FloatValue(10.0));
	row.insert(olive::BlurFilterNode::kDirectionalDegreesInput,
			   FloatValue(45.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	// Directional blur ignores the axis toggles and always runs once.
	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);
	EXPECT_EQ(job->GetIterationCount(), 1);
	EXPECT_DOUBLE_EQ(
		job->GetValues()
			.value(olive::BlurFilterNode::kDirectionalDegreesInput)
			.toDouble(),
		45.0);
}

TEST(BlurFilterNode, RadialGizmoFollowsCenterAndHalfResolution)
{
	olive::BlurFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();

	ASSERT_EQ(node.GetGizmos().size(), 1);
	auto *gizmo = static_cast<olive::PointGizmo *>(node.GetGizmos().first());
	ASSERT_NE(gizmo, nullptr);

	olive::NodeValueRow row =
		MakeTextureRow(olive::BlurFilterNode::kTextureInput, tex);
	row.insert(olive::BlurFilterNode::kMethodInput,
			   ComboValue(int(olive::BlurFilterNode::kRadial)));
	row.insert(olive::BlurFilterNode::kRadialCenterInput,
			   Vec2Value(QVector2D(3.0f, -2.0f)));

	node.UpdateGizmoPositions(row, olive::NodeGlobals());

	// The gizmo sits at the center offset from half the texture resolution.
	EXPECT_TRUE(gizmo->IsVisible());
	EXPECT_EQ(gizmo->GetPoint(), QPointF(11.0, 6.0));
	EXPECT_EQ(node.GetInputProperty(olive::BlurFilterNode::kRadialCenterInput,
									QStringLiteral("offset"))
				  .value<QVector2D>(),
			  QVector2D(8.0f, 8.0f));

	// Any other method hides the gizmo again.
	row[olive::BlurFilterNode::kMethodInput] =
		ComboValue(int(olive::BlurFilterNode::kGaussian));
	node.UpdateGizmoPositions(row, olive::NodeGlobals());
	EXPECT_FALSE(gizmo->IsVisible());
}

// -----------------------------------------------------------------------------
// DropShadowFilter
// -----------------------------------------------------------------------------

TEST(DropShadowFilter, InputDefinitionsAndDefaults)
{
	olive::DropShadowFilter node;

	EXPECT_EQ(int(node.GetInputDataType(olive::DropShadowFilter::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::DropShadowFilter::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::DropShadowFilter::kTextureInput);

	// The default shadow is black.
	const olive::core::Color color =
		node.GetStandardValue(olive::DropShadowFilter::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 0.0f);
	EXPECT_FLOAT_EQ(color.green(), 0.0f);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::DropShadowFilter::kDistanceInput)
			.toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::DropShadowFilter::kAngleInput).toDouble(),
		135.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::DropShadowFilter::kSoftnessInput)
			.toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::DropShadowFilter::kSoftnessInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::DropShadowFilter::kOpacityInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::DropShadowFilter::kOpacityInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);
	EXPECT_EQ(node.GetInputProperty(olive::DropShadowFilter::kOpacityInput,
									QStringLiteral("view"))
				  .toInt(),
			  int(olive::FloatSlider::kPercentage));
	EXPECT_FALSE(
		node.GetStandardValue(olive::DropShadowFilter::kFastInput).toBool());
}

TEST(DropShadowFilter, Identity)
{
	olive::DropShadowFilter node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.dropshadow"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryFilter));
}

TEST(DropShadowFilter, RetranslateSetsInputNames)
{
	olive::DropShadowFilter node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kColorInput),
			  QStringLiteral("Color"));
	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kDistanceInput),
			  QStringLiteral("Distance"));
	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kAngleInput),
			  QStringLiteral("Angle"));
	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kSoftnessInput),
			  QStringLiteral("Softness"));
	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kOpacityInput),
			  QStringLiteral("Opacity"));
	EXPECT_EQ(node.GetInputName(olive::DropShadowFilter::kFastInput),
			  QStringLiteral("Faster (Lower Quality)"));
}

TEST(DropShadowFilter, ShaderCodeLoadsFragmentResource)
{
	olive::DropShadowFilter node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(DropShadowFilter, ValueWithoutTexturePushesNothing)
{
	olive::DropShadowFilter node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(DropShadowFilter, ValueWithZeroSoftnessPushesSingleIterationJob)
{
	olive::DropShadowFilter node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::DropShadowFilter::kTextureInput, tex);
	row.insert(olive::DropShadowFilter::kSoftnessInput, FloatValue(0.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// Zero softness skips the blur passes: a single shader iteration remains.
	EXPECT_EQ(job->GetIterationCount(), 1);

	const olive::NodeValueRow &values = job->GetValues();
	EXPECT_EQ(values.value(QStringLiteral("resolution_in")).toVec2(),
			  QVector2D(16.0f, 16.0f));
	// The previous-iteration input is always seeded with the source texture.
	EXPECT_EQ(values.value(QStringLiteral("previous_iteration_in"))
				  .toTexture(),
			  tex);
}

TEST(DropShadowFilter, ValueWithSoftnessPushesThreeIterationJob)
{
	olive::DropShadowFilter node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::DropShadowFilter::kTextureInput, tex);
	row.insert(olive::DropShadowFilter::kSoftnessInput, FloatValue(10.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// Non-zero softness blurs iteratively over the previous pass.
	EXPECT_EQ(job->GetIterationCount(), 3);
	EXPECT_EQ(job->GetIterativeInput(),
			  QStringLiteral("previous_iteration_in"));
}

// -----------------------------------------------------------------------------
// MosaicFilterNode
// -----------------------------------------------------------------------------

TEST(MosaicFilterNode, InputDefinitionsAndDefaults)
{
	olive::MosaicFilterNode node;

	EXPECT_EQ(
		int(node.GetInputDataType(olive::MosaicFilterNode::kTextureInput)),
		int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::MosaicFilterNode::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::MosaicFilterNode::kTextureInput);

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::MosaicFilterNode::kHorizInput).toDouble(),
		32.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::MosaicFilterNode::kHorizInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 1.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::MosaicFilterNode::kVertInput).toDouble(),
		18.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::MosaicFilterNode::kVertInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 1.0);
}

TEST(MosaicFilterNode, Identity)
{
	olive::MosaicFilterNode node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.mosaicfilter"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryFilter));
}

TEST(MosaicFilterNode, RetranslateSetsInputNames)
{
	olive::MosaicFilterNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::MosaicFilterNode::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::MosaicFilterNode::kHorizInput),
			  QStringLiteral("Horizontal"));
	EXPECT_EQ(node.GetInputName(olive::MosaicFilterNode::kVertInput),
			  QStringLiteral("Vertical"));
}

TEST(MosaicFilterNode, ShaderCodeLoadsFragmentResource)
{
	olive::MosaicFilterNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(MosaicFilterNode, ValueWithoutTexturePushesNothing)
{
	olive::MosaicFilterNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(MosaicFilterNode, ValueWithMatchingResolutionPassesTextureThrough)
{
	olive::MosaicFilterNode node;

	// A mosaic block size equal to the texture size is a no-op.
	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::MosaicFilterNode::kTextureInput, tex);
	row.insert(olive::MosaicFilterNode::kHorizInput, FloatValue(16.0));
	row.insert(olive::MosaicFilterNode::kVertInput, FloatValue(16.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(MosaicFilterNode, ValueWithSingleAxisMatchingResolutionRunsJob)
{
	olive::MosaicFilterNode node;

	// Only one axis matching the texture size still changes the image, so
	// the effect must run; passthrough requires BOTH axes to match.
	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::MosaicFilterNode::kTextureInput, tex);
	row.insert(olive::MosaicFilterNode::kHorizInput, FloatValue(16.0));
	row.insert(olive::MosaicFilterNode::kVertInput, FloatValue(8.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_TRUE(out->IsJob());
}

TEST(MosaicFilterNode, ValuePushesJobWithLinearInterpolation)
{
	olive::MosaicFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::MosaicFilterNode::kTextureInput, tex);
	row.insert(olive::MosaicFilterNode::kHorizInput, FloatValue(32.0));
	row.insert(olive::MosaicFilterNode::kVertInput, FloatValue(18.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// Mipmapping would smear the blocks, so the mosaic forces bilinear lookup.
	EXPECT_EQ(int(job->GetInterpolation(olive::MosaicFilterNode::kTextureInput)),
			  int(olive::Texture::kLinear));
}

// -----------------------------------------------------------------------------
// StrokeFilterNode
// -----------------------------------------------------------------------------

TEST(StrokeFilterNode, InputDefinitionsAndDefaults)
{
	olive::StrokeFilterNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::StrokeFilterNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::StrokeFilterNode::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::StrokeFilterNode::kTextureInput);

	// The default stroke is opaque white.
	const olive::core::Color color =
		node.GetStandardValue(olive::StrokeFilterNode::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 1.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::StrokeFilterNode::kRadiusInput).toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::StrokeFilterNode::kRadiusInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::StrokeFilterNode::kOpacityInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::StrokeFilterNode::kOpacityInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::StrokeFilterNode::kOpacityInput,
							  QStringLiteral("max"))
			.toDouble(),
		1.0);
	EXPECT_EQ(node.GetInputProperty(olive::StrokeFilterNode::kOpacityInput,
									QStringLiteral("view"))
				  .toInt(),
			  int(olive::FloatSlider::kPercentage));
	EXPECT_FALSE(
		node.GetStandardValue(olive::StrokeFilterNode::kInnerInput).toBool());
}

TEST(StrokeFilterNode, Identity)
{
	olive::StrokeFilterNode node;

	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.stroke"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryFilter));
}

TEST(StrokeFilterNode, RetranslateSetsInputNames)
{
	olive::StrokeFilterNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::StrokeFilterNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::StrokeFilterNode::kColorInput),
			  QStringLiteral("Color"));
	EXPECT_EQ(node.GetInputName(olive::StrokeFilterNode::kRadiusInput),
			  QStringLiteral("Radius"));
	EXPECT_EQ(node.GetInputName(olive::StrokeFilterNode::kOpacityInput),
			  QStringLiteral("Opacity"));
	EXPECT_EQ(node.GetInputName(olive::StrokeFilterNode::kInnerInput),
			  QStringLiteral("Inner"));
}

TEST(StrokeFilterNode, ShaderCodeLoadsFragmentResource)
{
	olive::StrokeFilterNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(StrokeFilterNode, ValueWithoutTexturePushesNothing)
{
	olive::StrokeFilterNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(StrokeFilterNode, ValueWithRadiusAndOpacityPushesJob)
{
	olive::StrokeFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::StrokeFilterNode::kTextureInput, tex);
	row.insert(olive::StrokeFilterNode::kRadiusInput, FloatValue(10.0));
	row.insert(olive::StrokeFilterNode::kOpacityInput, FloatValue(1.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);
	EXPECT_EQ(job->GetValues()
				  .value(QStringLiteral("resolution_in"))
				  .toVec2(),
			  QVector2D(16.0f, 16.0f));
}

TEST(StrokeFilterNode, ValueWithZeroRadiusPassesTextureThrough)
{
	olive::StrokeFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::StrokeFilterNode::kTextureInput, tex);
	row.insert(olive::StrokeFilterNode::kRadiusInput, FloatValue(0.0));
	row.insert(olive::StrokeFilterNode::kOpacityInput, FloatValue(1.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(StrokeFilterNode, ValueWithZeroOpacityPassesTextureThrough)
{
	olive::StrokeFilterNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::StrokeFilterNode::kTextureInput, tex);
	row.insert(olive::StrokeFilterNode::kRadiusInput, FloatValue(10.0));
	row.insert(olive::StrokeFilterNode::kOpacityInput, FloatValue(0.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

// -----------------------------------------------------------------------------
// ChromaKeyNode
// -----------------------------------------------------------------------------

TEST(ChromaKeyNode, InputDefinitionsAndDefaults)
{
	olive::ChromaKeyNode node;

	// The texture input comes from OCIOBaseNode and is the effect input.
	EXPECT_TRUE(node.HasInputWithID(olive::OCIOBaseNode::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::OCIOBaseNode::kTextureInput);

	// The default key color is pure green.
	const olive::core::Color color =
		node.GetStandardValue(olive::ChromaKeyNode::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 0.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ChromaKeyNode::kLowerToleranceInput)
			.toDouble(),
		5.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ChromaKeyNode::kLowerToleranceInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ChromaKeyNode::kLowerToleranceInput,
							  QStringLiteral("base"))
			.toDouble(),
		0.1);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ChromaKeyNode::kUpperToleranceInput)
			.toDouble(),
		25.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ChromaKeyNode::kUpperToleranceInput,
							  QStringLiteral("base"))
			.toDouble(),
		0.1);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ChromaKeyNode::kHighlightsInput)
			.toDouble(),
		100.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ChromaKeyNode::kShadowsInput).toDouble(),
		100.0);

	EXPECT_EQ(int(node.GetInputDataType(
					  olive::ChromaKeyNode::kGarbageMatteInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::ChromaKeyNode::kGarbageMatteInput));
	EXPECT_EQ(
		int(node.GetInputDataType(olive::ChromaKeyNode::kCoreMatteInput)),
		int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::ChromaKeyNode::kCoreMatteInput));

	EXPECT_FALSE(
		node.GetStandardValue(olive::ChromaKeyNode::kInvertInput).toBool());
	EXPECT_FALSE(
		node.GetStandardValue(olive::ChromaKeyNode::kMaskOnlyInput).toBool());
}

TEST(ChromaKeyNode, Identity)
{
	olive::ChromaKeyNode node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.chromakey"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryKeying));
}

TEST(ChromaKeyNode, RetranslateSetsInputNames)
{
	olive::ChromaKeyNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::OCIOBaseNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kGarbageMatteInput),
			  QStringLiteral("Garbage Matte"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kCoreMatteInput),
			  QStringLiteral("Core Matte"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kColorInput),
			  QStringLiteral("Key Color"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kShadowsInput),
			  QStringLiteral("Shadows"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kHighlightsInput),
			  QStringLiteral("Highlights"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kUpperToleranceInput),
			  QStringLiteral("Upper Tolerance"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kLowerToleranceInput),
			  QStringLiteral("Lower Tolerance"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kInvertInput),
			  QStringLiteral("Invert Mask"));
	EXPECT_EQ(node.GetInputName(olive::ChromaKeyNode::kMaskOnlyInput),
			  QStringLiteral("Show Mask Only"));
}

TEST(ChromaKeyNode, ShaderCodeSubstitutesStub)
{
	olive::ChromaKeyNode node;

	// The fragment shader contains a %1 placeholder for OCIO-generated code,
	// which GetShaderCode fills with the request's stub.
	const olive::ShaderCode with_stub = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("test"),
								   QStringLiteral("OAK_TEST_STUB")));
	EXPECT_TRUE(with_stub.frag_code().contains(QStringLiteral("OAK_TEST_STUB")));

	const olive::ShaderCode no_stub =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));
	EXPECT_FALSE(no_stub.frag_code().isEmpty());
	EXPECT_FALSE(no_stub.frag_code().contains(QStringLiteral("%1")));
}

TEST(ChromaKeyNode, ValueWithoutProcessorPushesNothing)
{
	// Without a project no color manager is attached, so no processor is ever
	// generated and Value() must push nothing even with a valid texture.
	olive::ChromaKeyNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OCIOBaseNode::kTextureInput, tex);

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(ChromaKeyNode, ValueInProjectPushesColorTransformJob)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *node = new olive::ChromaKeyNode();
	node->setParent(&project);

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OCIOBaseNode::kTextureInput, tex);

	olive::NodeValueTable table;
	node->Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ColorTransformJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// Adding the node to the project generates its XYZ processor.
	EXPECT_NE(job->GetColorProcessor(), nullptr);
	EXPECT_EQ(job->GetFunctionName(),
			  QStringLiteral("SceneLinearToCIEXYZ_d65"));
	EXPECT_EQ(job->CustomShaderSource(), node);
	EXPECT_EQ(job->GetInputTexture().toTexture(), tex);
}

// -----------------------------------------------------------------------------
// ColorDifferenceKeyNode
// -----------------------------------------------------------------------------

TEST(ColorDifferenceKeyNode, InputDefinitionsAndDefaults)
{
	olive::ColorDifferenceKeyNode node;

	EXPECT_EQ(int(node.GetInputDataType(
					  olive::ColorDifferenceKeyNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(
		olive::ColorDifferenceKeyNode::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(),
			  olive::ColorDifferenceKeyNode::kTextureInput);

	EXPECT_EQ(int(node.GetInputDataType(
					  olive::ColorDifferenceKeyNode::kGarbageMatteInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_EQ(int(node.GetInputDataType(
					  olive::ColorDifferenceKeyNode::kCoreMatteInput)),
			  int(olive::NodeValue::kTexture));

	// Key color is a static combo defaulting to the first entry (green).
	EXPECT_EQ(int(node.GetInputDataType(
					  olive::ColorDifferenceKeyNode::kColorInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node.GetStandardValue(olive::ColorDifferenceKeyNode::kColorInput)
				  .toInt(),
			  0);

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ColorDifferenceKeyNode::kHighlightsInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ColorDifferenceKeyNode::kHighlightsInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ColorDifferenceKeyNode::kHighlightsInput,
							  QStringLiteral("base"))
			.toDouble(),
		0.01);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ColorDifferenceKeyNode::kShadowsInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ColorDifferenceKeyNode::kShadowsInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ColorDifferenceKeyNode::kShadowsInput,
							  QStringLiteral("base"))
			.toDouble(),
		0.01);

	EXPECT_FALSE(node.GetStandardValue(
					  olive::ColorDifferenceKeyNode::kMaskOnlyInput)
					 .toBool());
}

TEST(ColorDifferenceKeyNode, Identity)
{
	olive::ColorDifferenceKeyNode node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.colordifferencekey"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryKeying));
}

TEST(ColorDifferenceKeyNode, RetranslateSetsInputNamesAndComboStrings)
{
	olive::ColorDifferenceKeyNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::ColorDifferenceKeyNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(
		node.GetInputName(olive::ColorDifferenceKeyNode::kGarbageMatteInput),
		QStringLiteral("Garbage Matte"));
	EXPECT_EQ(node.GetInputName(olive::ColorDifferenceKeyNode::kCoreMatteInput),
			  QStringLiteral("Core Matte"));
	EXPECT_EQ(node.GetInputName(olive::ColorDifferenceKeyNode::kColorInput),
			  QStringLiteral("Key Color"));
	EXPECT_EQ(node.GetComboBoxStrings(
				  olive::ColorDifferenceKeyNode::kColorInput),
			  QStringList(
				  { QStringLiteral("Green"), QStringLiteral("Blue") }));
	EXPECT_EQ(node.GetInputName(olive::ColorDifferenceKeyNode::kShadowsInput),
			  QStringLiteral("Shadows"));
	EXPECT_EQ(
		node.GetInputName(olive::ColorDifferenceKeyNode::kHighlightsInput),
		QStringLiteral("Highlights"));
	EXPECT_EQ(node.GetInputName(olive::ColorDifferenceKeyNode::kMaskOnlyInput),
			  QStringLiteral("Show Mask Only"));
}

TEST(ColorDifferenceKeyNode, ShaderCodeLoadsFragmentResource)
{
	olive::ColorDifferenceKeyNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(ColorDifferenceKeyNode, ValueWithoutTexturePushesNothing)
{
	olive::ColorDifferenceKeyNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(ColorDifferenceKeyNode, ValuePushesShaderJobWithRowValues)
{
	olive::ColorDifferenceKeyNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::ColorDifferenceKeyNode::kTextureInput, tex);
	row.insert(olive::ColorDifferenceKeyNode::kColorInput, ComboValue(1));
	row.insert(olive::ColorDifferenceKeyNode::kMaskOnlyInput, BoolValue(true));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// The whole input row is forwarded into the job.
	const olive::NodeValueRow &values = job->GetValues();
	EXPECT_EQ(values.value(olive::ColorDifferenceKeyNode::kTextureInput)
				  .toTexture(),
			  tex);
	EXPECT_EQ(values.value(olive::ColorDifferenceKeyNode::kColorInput).toInt(),
			  1);
	EXPECT_TRUE(values.value(olive::ColorDifferenceKeyNode::kMaskOnlyInput)
					.toBool());
}

// -----------------------------------------------------------------------------
// DespillNode
// -----------------------------------------------------------------------------

TEST(DespillNode, InputDefinitionsAndDefaults)
{
	olive::DespillNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::DespillNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::DespillNode::kTextureInput));
	EXPECT_EQ(node.GetEffectInputID(), olive::DespillNode::kTextureInput);

	EXPECT_EQ(int(node.GetInputDataType(olive::DespillNode::kColorInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node.GetStandardValue(olive::DespillNode::kColorInput).toInt(),
			  0);
	EXPECT_EQ(int(node.GetInputDataType(olive::DespillNode::kMethodInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node.GetStandardValue(olive::DespillNode::kMethodInput).toInt(),
			  0);
	EXPECT_FALSE(node.GetStandardValue(
					  olive::DespillNode::kPreserveLuminanceInput)
					 .toBool());
}

TEST(DespillNode, Identity)
{
	olive::DespillNode node;

	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.despill"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryKeying));
}

TEST(DespillNode, RetranslateSetsInputNamesAndComboStrings)
{
	olive::DespillNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::DespillNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::DespillNode::kColorInput),
			  QStringLiteral("Key Color"));
	EXPECT_EQ(node.GetComboBoxStrings(olive::DespillNode::kColorInput),
			  QStringList(
				  { QStringLiteral("Green"), QStringLiteral("Blue") }));
	EXPECT_EQ(node.GetInputName(olive::DespillNode::kMethodInput),
			  QStringLiteral("Method"));
	EXPECT_EQ(node.GetComboBoxStrings(olive::DespillNode::kMethodInput),
			  QStringList({ QStringLiteral("Average"),
							QStringLiteral("Double Red Average"),
							QStringLiteral("Double Average"),
							QStringLiteral("Limit") }));
	EXPECT_EQ(node.GetInputName(olive::DespillNode::kPreserveLuminanceInput),
			  QStringLiteral("Preserve Luminance"));
}

TEST(DespillNode, ShaderCodeLoadsFragmentResource)
{
	olive::DespillNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(DespillNode, ValueInProjectWithoutTexturePushesNothing)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *node = new olive::DespillNode();
	node->setParent(&project);

	olive::NodeValueTable table;
	node->Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(DespillNode, ValueInProjectPushesJobWithLumaCoefficients)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *node = new olive::DespillNode();
	node->setParent(&project);

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::DespillNode::kTextureInput, tex);

	olive::NodeValueTable table;
	node->Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	// The job carries the color manager's default luma coefficients.
	double expected[3] = { 0.0, 0.0, 0.0 };
	project.color_manager()->GetDefaultLumaCoefs(expected);

	const olive::NodeValueRow &values = job->GetValues();
	ASSERT_TRUE(values.contains(QStringLiteral("luma_coeffs")));
	const QVector3D coeffs =
		values.value(QStringLiteral("luma_coeffs")).toVec3();
	EXPECT_FLOAT_EQ(coeffs.x(), float(expected[0]));
	EXPECT_FLOAT_EQ(coeffs.y(), float(expected[1]));
	EXPECT_FLOAT_EQ(coeffs.z(), float(expected[2]));

	EXPECT_EQ(values.value(olive::DespillNode::kTextureInput).toTexture(),
			  tex);
}

TEST(DespillNode, ValueWithoutProjectUsesRec709LumaFallback)
{
	// A graph-less node has no project color manager; it must fall back to
	// Rec. 709 luma coefficients instead of crashing.
	olive::DespillNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::DespillNode::kTextureInput, tex);

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ShaderJob *>(out->job());
	ASSERT_NE(job, nullptr);

	const olive::NodeValueRow &values = job->GetValues();
	ASSERT_TRUE(values.contains(QStringLiteral("luma_coeffs")));
	const QVector3D coeffs =
		values.value(QStringLiteral("luma_coeffs")).toVec3();
	EXPECT_NEAR(coeffs.x(), 0.2126f, 0.0001f);
	EXPECT_NEAR(coeffs.y(), 0.7152f, 0.0001f);
	EXPECT_NEAR(coeffs.z(), 0.0722f, 0.0001f);
}
