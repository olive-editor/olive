#include <gtest/gtest.h>

#include <QStringList>
#include <QVector3D>
#include <QVector4D>

#include "node/color/colormanager/colormanager.h"
#include "node/color/displaytransform/displaytransform.h"
#include "node/color/ociobase/ociobase.h"
#include "node/color/ociogradingtransformlinear/ociogradingtransformlinear.h"
#include "node/color/threewaycolor/threewaycolor.h"
#include "node/project.h"
#include "render/job/colortransformjob.h"
#include "render/job/shaderjob.h"
#include "render/texture.h"

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

olive::NodeValueRow MakeTextureRow(const QString &input,
								   const olive::TexturePtr &tex)
{
	olive::NodeValueRow row;
	row.insert(input, olive::NodeValue(olive::NodeValue::kTexture, tex));
	return row;
}

olive::NodeValue Vec4Value(const QVector4D &v)
{
	return olive::NodeValue(olive::NodeValue::kVec4, v);
}

olive::NodeValue BoolValue(bool b)
{
	return olive::NodeValue(olive::NodeValue::kBoolean, b);
}

} // namespace

// -----------------------------------------------------------------------------
// OCIOBaseNode passthrough (exercised through DisplayTransformNode, which is
// a concrete OCIOBaseNode). Without a Project the base never has a processor,
// so Value() must pass the input texture through unchanged.
// -----------------------------------------------------------------------------

TEST(OCIOBaseNode, PassesTextureThroughWhenProcessorMissing)
{
	olive::DisplayTransformNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OCIOBaseNode::kTextureInput, tex);

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::NodeValue out = table.Get(olive::NodeValue::kTexture);
	EXPECT_EQ(out.type(), olive::NodeValue::kTexture);
	EXPECT_EQ(out.toTexture(), tex);
}

TEST(OCIOBaseNode, PushesNothingWhenTextureInputEmpty)
{
	olive::DisplayTransformNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

// -----------------------------------------------------------------------------
// DisplayTransformNode
// -----------------------------------------------------------------------------

TEST(DisplayTransformNode, InputDefinitions)
{
	olive::DisplayTransformNode node;

	EXPECT_TRUE(node.HasInputWithID(olive::OCIOBaseNode::kTextureInput));
	EXPECT_TRUE(node.HasInputWithID(olive::DisplayTransformNode::kDisplayInput));
	EXPECT_TRUE(node.HasInputWithID(olive::DisplayTransformNode::kViewInput));
	EXPECT_TRUE(
		node.HasInputWithID(olive::DisplayTransformNode::kDirectionInput));

	EXPECT_EQ(node.GetInputDataType(olive::DisplayTransformNode::kDisplayInput),
			  olive::NodeValue::kCombo);
	EXPECT_EQ(node.GetInputDataType(olive::DisplayTransformNode::kViewInput),
			  olive::NodeValue::kCombo);
	EXPECT_EQ(
		node.GetInputDataType(olive::DisplayTransformNode::kDirectionInput),
		olive::NodeValue::kCombo);

	// Combo inputs are static UI choices: neither keyframable nor connectable.
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::DisplayTransformNode::kDisplayInput));
	EXPECT_FALSE(
		node.IsInputConnectable(olive::DisplayTransformNode::kDisplayInput));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::DisplayTransformNode::kViewInput));
	EXPECT_FALSE(
		node.IsInputConnectable(olive::DisplayTransformNode::kViewInput));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::DisplayTransformNode::kDirectionInput));
	EXPECT_FALSE(
		node.IsInputConnectable(olive::DisplayTransformNode::kDirectionInput));

	EXPECT_EQ(node.GetStandardValue(olive::DisplayTransformNode::kDisplayInput)
				  .toInt(),
			  0);
	EXPECT_EQ(node.GetStandardValue(olive::DisplayTransformNode::kViewInput)
				  .toInt(),
			  0);
	EXPECT_EQ(node.GetStandardValue(olive::DisplayTransformNode::kDirectionInput)
				  .toInt(),
			  0);

	EXPECT_EQ(node.GetEffectInputID(), olive::OCIOBaseNode::kTextureInput);
}

TEST(DisplayTransformNode, Identity)
{
	olive::DisplayTransformNode node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.displaytransform"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryColor));
}

TEST(DisplayTransformNode, DisplayAndViewEmptyWithoutProject)
{
	olive::DisplayTransformNode node;

	// No ColorManager is attached, so display/view cannot be resolved.
	EXPECT_TRUE(node.GetDisplay().isEmpty());
	EXPECT_TRUE(node.GetView().isEmpty());
	EXPECT_EQ(int(node.GetDirection()), int(olive::ColorProcessor::kNormal));
}

TEST(DisplayTransformNode, ResolvesDisplayAndViewInProject)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	olive::ColorManager *manager = project.color_manager();
	ASSERT_NE(manager, nullptr);

	const QStringList displays = manager->ListAvailableDisplays();
	ASSERT_FALSE(displays.isEmpty());

	auto *node = new olive::DisplayTransformNode();
	node->setParent(&project);

	// Combo index 0 must resolve to the first available display/view.
	EXPECT_EQ(node->GetDisplay(), displays.first());

	const QStringList views = manager->ListAvailableViews(node->GetDisplay());
	ASSERT_FALSE(views.isEmpty());
	EXPECT_EQ(node->GetView(), views.first());

	EXPECT_EQ(int(node->GetDirection()), int(olive::ColorProcessor::kNormal));

	node->SetStandardValue(olive::DisplayTransformNode::kDirectionInput, 1);
	EXPECT_EQ(int(node->GetDirection()), int(olive::ColorProcessor::kInverse));
}

TEST(DisplayTransformNode, RetranslateSetsInputNames)
{
	olive::DisplayTransformNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::OCIOBaseNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::DisplayTransformNode::kDisplayInput),
			  QStringLiteral("Display"));
	EXPECT_EQ(node.GetInputName(olive::DisplayTransformNode::kViewInput),
			  QStringLiteral("View"));
	EXPECT_EQ(node.GetInputName(olive::DisplayTransformNode::kDirectionInput),
			  QStringLiteral("Direction"));
}

// -----------------------------------------------------------------------------
// ThreeWayColorNode (beyond the factory/default coverage in color_lut_test.cpp)
// -----------------------------------------------------------------------------

TEST(ThreeWayColorNode, ShaderCodeLoadsFragmentResource)
{
	olive::ThreeWayColorNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("test")));

	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(ThreeWayColorNode, ValueWithoutTexturePushesNothing)
{
	olive::ThreeWayColorNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(ThreeWayColorNode, ValuePushesShaderJobWithDefaultLumaCoefficients)
{
	olive::ThreeWayColorNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::ThreeWayColorNode::kTextureInput, tex);

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
	EXPECT_TRUE(values.contains(olive::ThreeWayColorNode::kTextureInput));
	ASSERT_TRUE(values.contains(olive::ThreeWayColorNode::kLumaCoefficientsInput));

	// Without a project the node falls back to Rec. 709 luma coefficients.
	const QVector3D coeffs =
		values.value(olive::ThreeWayColorNode::kLumaCoefficientsInput).toVec3();
	EXPECT_NEAR(coeffs.x(), 0.2126f, 0.0001f);
	EXPECT_NEAR(coeffs.y(), 0.7152f, 0.0001f);
	EXPECT_NEAR(coeffs.z(), 0.0722f, 0.0001f);
}

TEST(ThreeWayColorNode, AmountInputsDefaultToFull)
{
	olive::ThreeWayColorNode node;

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ThreeWayColorNode::kShadowsAmountInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ThreeWayColorNode::kMidtonesAmountInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::ThreeWayColorNode::kHighlightsAmountInput)
			.toDouble(),
		1.0);

	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ThreeWayColorNode::kShadowsAmountInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ThreeWayColorNode::kMidtonesAmountInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(olive::ThreeWayColorNode::kHighlightsAmountInput,
							  QStringLiteral("min"))
			.toDouble(),
		0.0);
}

TEST(ThreeWayColorNode, RetranslateSetsInputNames)
{
	olive::ThreeWayColorNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::ThreeWayColorNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::ThreeWayColorNode::kShadowsColorInput),
			  QStringLiteral("Shadows"));
	EXPECT_EQ(node.GetInputName(olive::ThreeWayColorNode::kMidtonesColorInput),
			  QStringLiteral("Midtones"));
	EXPECT_EQ(
		node.GetInputName(olive::ThreeWayColorNode::kHighlightsColorInput),
		QStringLiteral("Highlights"));
	EXPECT_EQ(
		node.GetInputName(olive::ThreeWayColorNode::kShadowsAmountInput),
		QStringLiteral("Shadows Amount"));
	EXPECT_EQ(
		node.GetInputName(olive::ThreeWayColorNode::kMidtonesAmountInput),
		QStringLiteral("Midtones Amount"));
	EXPECT_EQ(
		node.GetInputName(olive::ThreeWayColorNode::kHighlightsAmountInput),
		QStringLiteral("Highlights Amount"));
}

// -----------------------------------------------------------------------------
// OCIOGradingTransformLinearNode (beyond the clamp-invariant coverage in
// color_lut_test.cpp)
// -----------------------------------------------------------------------------

TEST(GradingTransformLinear, InputDefaults)
{
	olive::OCIOGradingTransformLinearNode node;

	const QVector4D contrast =
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kContrastInput)
			.value<QVector4D>();
	EXPECT_FLOAT_EQ(contrast.x(), 1.0f);
	EXPECT_FLOAT_EQ(contrast.y(), 1.0f);
	EXPECT_FLOAT_EQ(contrast.z(), 1.0f);
	EXPECT_FLOAT_EQ(contrast.w(), 1.0f);

	const QVector4D offset =
		node.GetStandardValue(olive::OCIOGradingTransformLinearNode::kOffsetInput)
			.value<QVector4D>();
	EXPECT_FLOAT_EQ(offset.x(), 0.0f);
	EXPECT_FLOAT_EQ(offset.y(), 0.0f);
	EXPECT_FLOAT_EQ(offset.z(), 0.0f);
	EXPECT_FLOAT_EQ(offset.w(), 0.0f);

	const QVector4D exposure =
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kExposureInput)
			.value<QVector4D>();
	EXPECT_FLOAT_EQ(exposure.x(), 0.0f);
	EXPECT_FLOAT_EQ(exposure.y(), 0.0f);
	EXPECT_FLOAT_EQ(exposure.z(), 0.0f);
	EXPECT_FLOAT_EQ(exposure.w(), 0.0f);

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kSaturationInput)
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::OCIOGradingTransformLinearNode::kPivotInput)
			.toDouble(),
		0.18);

	EXPECT_FALSE(
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput)
			.toBool());
	EXPECT_FALSE(
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput)
			.toBool());
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kClampBlackInput)
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput)
			.toDouble(),
		1.0);

	// Clamp value inputs start out disabled, matching the enable toggles.
	EXPECT_FALSE(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampBlackInput,
				QStringLiteral("enabled"))
			.toBool());
	EXPECT_FALSE(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
				QStringLiteral("enabled"))
			.toBool());
}

TEST(GradingTransformLinear, Identity)
{
	olive::OCIOGradingTransformLinearNode node;

	EXPECT_EQ(node.id(),
			  QStringLiteral(
				  "org.olivevideoeditor.Olive.ociogradingtransformlinear"));
	EXPECT_FALSE(node.Name().isEmpty());
	EXPECT_FALSE(node.Description().isEmpty());

	ASSERT_EQ(node.Category().size(), 1);
	EXPECT_EQ(int(node.Category().first()), int(olive::Node::kCategoryColor));
}

TEST(GradingTransformLinear, ClampEnableTogglesEnabledProperty)
{
	olive::OCIOGradingTransformLinearNode node;

	node.SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput, true);
	EXPECT_TRUE(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
				QStringLiteral("enabled"))
			.toBool());

	node.SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput, true);
	EXPECT_TRUE(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampBlackInput,
				QStringLiteral("enabled"))
			.toBool());

	node.SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput, false);
	EXPECT_FALSE(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
				QStringLiteral("enabled"))
			.toBool());
}

TEST(GradingTransformLinear, WhiteClampMinimumFollowsStaticBlackClamp)
{
	olive::OCIOGradingTransformLinearNode node;

	// Constructor seeds the white clamp minimum just above the black clamp.
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
				QStringLiteral("min"))
			.toDouble(),
		0.000001);

	node.SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackInput, 0.5);
	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
				QStringLiteral("min"))
			.toDouble(),
		0.5 + 0.000001);
}

TEST(GradingTransformLinear, WhiteClampMinimumNotUpdatedWhenBlackKeyframed)
{
	olive::OCIOGradingTransformLinearNode node;

	// With the black clamp keyframing, the static UI minimum can no longer
	// follow it; the invariant is enforced per frame in Value() instead.
	node.SetInputIsKeyframing(
		olive::OCIOGradingTransformLinearNode::kClampBlackInput, true);
	node.SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackInput, 0.5);

	EXPECT_DOUBLE_EQ(
		node.GetInputProperty(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
				QStringLiteral("min"))
			.toDouble(),
		0.000001);
}

TEST(GradingTransformLinear, ValueWithoutProcessorPushesNothing)
{
	// Without a project no color manager is attached, so no processor is ever
	// generated and Value() must push nothing even with a valid texture.
	olive::OCIOGradingTransformLinearNode node;

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OCIOBaseNode::kTextureInput, tex);

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(GradingTransformLinear, ValueInProjectPushesColorTransformJob)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *node = new olive::OCIOGradingTransformLinearNode();
	node->setParent(&project);

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OCIOBaseNode::kTextureInput, tex);
	row.insert(olive::OCIOGradingTransformLinearNode::kOffsetInput,
			   Vec4Value(QVector4D(0.0f, 0.0f, 0.0f, 0.0f)));
	row.insert(olive::OCIOGradingTransformLinearNode::kExposureInput,
			   Vec4Value(QVector4D(0.0f, 0.0f, 0.0f, 0.0f)));
	row.insert(olive::OCIOGradingTransformLinearNode::kContrastInput,
			   Vec4Value(QVector4D(1.0f, 1.0f, 1.0f, 1.0f)));
	row.insert(olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput,
			   BoolValue(false));
	row.insert(olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput,
			   BoolValue(false));

	olive::NodeValueTable table;
	node->Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ColorTransformJob *>(out->job());
	ASSERT_NE(job, nullptr);
	EXPECT_NE(job->GetColorProcessor(), nullptr);

	const olive::NodeValueRow &values = job->GetValues();

	// Defaults convert to neutral vec3s for OCIO.
	const QVector3D offset =
		values.value(olive::OCIOGradingTransformLinearNode::kOffsetInput)
			.toVec3();
	EXPECT_FLOAT_EQ(offset.x(), 0.0f);
	EXPECT_FLOAT_EQ(offset.y(), 0.0f);
	EXPECT_FLOAT_EQ(offset.z(), 0.0f);

	const QVector3D exposure =
		values.value(olive::OCIOGradingTransformLinearNode::kExposureInput)
			.toVec3();
	EXPECT_FLOAT_EQ(exposure.x(), 1.0f);
	EXPECT_FLOAT_EQ(exposure.y(), 1.0f);
	EXPECT_FLOAT_EQ(exposure.z(), 1.0f);

	const QVector3D contrast =
		values.value(olive::OCIOGradingTransformLinearNode::kContrastInput)
			.toVec3();
	EXPECT_FLOAT_EQ(contrast.x(), 1.0f);
	EXPECT_FLOAT_EQ(contrast.y(), 1.0f);
	EXPECT_FLOAT_EQ(contrast.z(), 1.0f);

	// Disabled clamps are replaced with OCIO's "no clamp" sentinels.
	ASSERT_TRUE(
		values.contains(olive::OCIOGradingTransformLinearNode::kClampBlackInput));
	EXPECT_LT(values
				  .value(olive::OCIOGradingTransformLinearNode::kClampBlackInput)
				  .toDouble(),
			  -1e300);
	ASSERT_TRUE(
		values.contains(olive::OCIOGradingTransformLinearNode::kClampWhiteInput));
	EXPECT_GT(values
				  .value(olive::OCIOGradingTransformLinearNode::kClampWhiteInput)
				  .toDouble(),
			  1e300);
}

TEST(GradingTransformLinear, ValueAppliesMasterChannelMath)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *node = new olive::OCIOGradingTransformLinearNode();
	node->setParent(&project);

	olive::TexturePtr tex = MakeDummyTexture();
	olive::NodeValueRow row =
		MakeTextureRow(olive::OCIOBaseNode::kTextureInput, tex);
	// Layout is {master, red, green, blue}.
	row.insert(olive::OCIOGradingTransformLinearNode::kOffsetInput,
			   Vec4Value(QVector4D(0.1f, 0.2f, 0.3f, 0.4f)));
	row.insert(olive::OCIOGradingTransformLinearNode::kExposureInput,
			   Vec4Value(QVector4D(1.0f, 0.0f, 0.0f, 0.0f)));
	row.insert(olive::OCIOGradingTransformLinearNode::kContrastInput,
			   Vec4Value(QVector4D(2.0f, 0.5f, 1.0f, 1.0f)));
	row.insert(olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput,
			   BoolValue(false));
	row.insert(olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput,
			   BoolValue(false));

	olive::NodeValueTable table;
	node->Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = static_cast<olive::ColorTransformJob *>(out->job());
	ASSERT_NE(job, nullptr);

	const olive::NodeValueRow &values = job->GetValues();

	// Offset: master is added to each channel.
	const QVector3D offset =
		values.value(olive::OCIOGradingTransformLinearNode::kOffsetInput)
			.toVec3();
	EXPECT_NEAR(offset.x(), 0.3f, 0.0001f);
	EXPECT_NEAR(offset.y(), 0.4f, 0.0001f);
	EXPECT_NEAR(offset.z(), 0.5f, 0.0001f);

	// Exposure: channels become 2^(master + channel) gain values.
	const QVector3D exposure =
		values.value(olive::OCIOGradingTransformLinearNode::kExposureInput)
			.toVec3();
	EXPECT_NEAR(exposure.x(), 2.0f, 0.0001f);
	EXPECT_NEAR(exposure.y(), 2.0f, 0.0001f);
	EXPECT_NEAR(exposure.z(), 2.0f, 0.0001f);

	// Contrast: master multiplies each channel.
	const QVector3D contrast =
		values.value(olive::OCIOGradingTransformLinearNode::kContrastInput)
			.toVec3();
	EXPECT_NEAR(contrast.x(), 1.0f, 0.0001f);
	EXPECT_NEAR(contrast.y(), 2.0f, 0.0001f);
	EXPECT_NEAR(contrast.z(), 2.0f, 0.0001f);
}

TEST(GradingTransformLinear, RetranslateSetsInputNames)
{
	olive::OCIOGradingTransformLinearNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::OCIOBaseNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(
		node.GetInputName(olive::OCIOGradingTransformLinearNode::kContrastInput),
		QStringLiteral("Contrast"));
	EXPECT_EQ(
		node.GetInputName(olive::OCIOGradingTransformLinearNode::kOffsetInput),
		QStringLiteral("Offset"));
	EXPECT_EQ(
		node.GetInputName(olive::OCIOGradingTransformLinearNode::kExposureInput),
		QStringLiteral("Exposure"));
	EXPECT_EQ(
		node.GetInputName(
			olive::OCIOGradingTransformLinearNode::kSaturationInput),
		QStringLiteral("Saturation"));
	EXPECT_EQ(
		node.GetInputName(olive::OCIOGradingTransformLinearNode::kPivotInput),
		QStringLiteral("Pivot"));
	EXPECT_EQ(
		node.GetInputName(
			olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput),
		QStringLiteral("Enable Black Clamp"));
	EXPECT_EQ(
		node.GetInputName(
			olive::OCIOGradingTransformLinearNode::kClampBlackInput),
		QStringLiteral("Black Clamp"));
	EXPECT_EQ(
		node.GetInputName(
			olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput),
		QStringLiteral("Enable White Clamp"));
	EXPECT_EQ(
		node.GetInputName(
			olive::OCIOGradingTransformLinearNode::kClampWhiteInput),
		QStringLiteral("White Clamp"));
}
