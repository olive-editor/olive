#include <gtest/gtest.h>

#include <QMatrix4x4>
#include <QPointF>
#include <QStringList>
#include <QVector2D>
#include <QVector3D>

#include "node/color/colormanager/colormanager.h"
#include "node/distort/cornerpin/cornerpindistortnode.h"
#include "node/distort/crop/cropdistortnode.h"
#include "node/distort/flip/flipdistortnode.h"
#include "node/distort/mask/mask.h"
#include "node/distort/ripple/rippledistortnode.h"
#include "node/distort/swirl/swirldistortnode.h"
#include "node/distort/tile/tiledistortnode.h"
#include "node/distort/transform/transformdistortnode.h"
#include "node/distort/wave/wavedistortnode.h"
#include "node/filter/blur/blur.h"
#include "node/generator/polygon/polygon.h"
#include "node/generator/shape/generatorwithmerge.h"
#include "node/globals.h"
#include "node/project.h"
#include "node/traverser.h"
#include "olive/core/util/color.h"
#include "render/job/generatejob.h"
#include "render/job/shaderjob.h"
#include "render/loopmode.h"
#include "render/texture.h"
#include "render/videoparams.h"
#include "widget/slider/floatslider.h"

namespace
{

// Node that pushes a fixed dummy texture, used to feed the texture input of
// distort nodes without any renderer (same pattern as node_generator_test).
class ConstantTextureNode : public olive::Node {
public:
	ConstantTextureNode() = default;

	NODE_DEFAULT_FUNCTIONS(ConstantTextureNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Test Texture");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.distort_texture");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryGenerator };
	}

	void SetTexture(const olive::TexturePtr &texture)
	{
		texture_ = texture;
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)
		Q_UNUSED(globals)

		table->Push(olive::NodeValue(olive::NodeValue::kTexture, texture_, this));
	}

private:
	olive::TexturePtr texture_;
};

template <typename T> T *AddNode(olive::Project *project)
{
	T *node = new T();
	node->setParent(project);
	return node;
}

olive::TimeRange FirstFrame()
{
	return olive::TimeRange(olive::rational(0), olive::rational(1, 30));
}

// A fresh traverser per call: NodeTraverser caches tables per node/range, so
// reusing one would return stale results after changing standard values.
olive::NodeValueTable GenerateTable(const olive::Node *node,
									const olive::VideoParams &vparams)
{
	olive::NodeTraverser traverser;
	traverser.SetCacheVideoParams(vparams);
	return traverser.GenerateTable(node, FirstFrame());
}

// A "dummy" texture has no renderer backend and is therefore safe to pass
// around in a headless, CPU-only test.
olive::TexturePtr MakeDummyTexture(int width, int height)
{
	return std::make_shared<olive::Texture>(
		olive::VideoParams(width, height, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));
}

olive::VideoParams SequenceParams(int width, int height)
{
	return olive::VideoParams(width, height, olive::core::PixelFormat::F32,
							  olive::VideoParams::kRGBAChannelCount);
}

olive::NodeValue TextureValue(const olive::TexturePtr &texture)
{
	return olive::NodeValue(olive::NodeValue::kTexture, texture);
}

olive::NodeValue FloatValue(double v)
{
	return olive::NodeValue(olive::NodeValue::kFloat, v);
}

olive::NodeValue BoolValue(bool b)
{
	return olive::NodeValue(olive::NodeValue::kBoolean, b);
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

olive::TexturePtr GetOutputTexture(const olive::NodeValueTable &table)
{
	return table.Get(olive::NodeValue::kTexture).toTexture();
}

} // namespace

// -----------------------------------------------------------------------------
// TransformDistortNode
// -----------------------------------------------------------------------------

TEST(TransformDistortNode, MetadataIsCorrect)
{
	olive::TransformDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.transform"));
	EXPECT_EQ(node.Name(), QStringLiteral("Transform"));
	// ShortName() overrides MatrixGenerator's "Ortho"
	EXPECT_EQ(node.ShortName(), QStringLiteral("Transform"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::TransformDistortNode::kTextureInput);
}

TEST(TransformDistortNode, InputDefinitionsAndDefaults)
{
	olive::TransformDistortNode node;

	// Texture is prepended, so it is the primary effect input and cannot be
	// keyframed
	ASSERT_TRUE(node.HasInputWithID(olive::TransformDistortNode::kTextureInput));
	EXPECT_EQ(int(node.GetInputDataType(olive::TransformDistortNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::TransformDistortNode::kTextureInput));

	ASSERT_TRUE(node.HasInputWithID(olive::TransformDistortNode::kParentInput));
	EXPECT_EQ(int(node.GetInputDataType(olive::TransformDistortNode::kParentInput)),
			  int(olive::NodeValue::kMatrix));

	ASSERT_TRUE(node.HasInputWithID(olive::TransformDistortNode::kAutoscaleInput));
	EXPECT_EQ(
		int(node.GetInputDataType(olive::TransformDistortNode::kAutoscaleInput)),
		int(olive::NodeValue::kCombo));
	EXPECT_EQ(node.GetStandardValue(olive::TransformDistortNode::kAutoscaleInput)
				  .toInt(),
			  int(olive::TransformDistortNode::kAutoScaleNone));

	ASSERT_TRUE(
		node.HasInputWithID(olive::TransformDistortNode::kInterpolationInput));
	EXPECT_EQ(int(node.GetInputDataType(
				  olive::TransformDistortNode::kInterpolationInput)),
			  int(olive::NodeValue::kCombo));
	// 2 = mipmapped bilinear
	EXPECT_EQ(node.GetStandardValue(
				  olive::TransformDistortNode::kInterpolationInput)
				  .toInt(),
			  int(olive::Texture::kMipmappedLinear));

	// MatrixGenerator inputs are inherited
	EXPECT_TRUE(node.HasInputWithID(olive::MatrixGenerator::kPositionInput));
	EXPECT_TRUE(node.HasInputWithID(olive::MatrixGenerator::kRotationInput));
	EXPECT_TRUE(node.HasInputWithID(olive::MatrixGenerator::kScaleInput));
	EXPECT_TRUE(node.HasInputWithID(olive::MatrixGenerator::kUniformScaleInput));
	EXPECT_TRUE(node.HasInputWithID(olive::MatrixGenerator::kAnchorInput));
}

TEST(TransformDistortNode, RetranslateSetsNamesAndComboStrings)
{
	olive::TransformDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::TransformDistortNode::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::TransformDistortNode::kParentInput),
			  QStringLiteral("Parent"));
	EXPECT_EQ(node.GetInputName(olive::TransformDistortNode::kAutoscaleInput),
			  QStringLiteral("Auto-Scale"));
	EXPECT_EQ(node.GetInputName(olive::TransformDistortNode::kInterpolationInput),
			  QStringLiteral("Interpolation"));

	// Inherited names from MatrixGenerator
	EXPECT_EQ(node.GetInputName(olive::MatrixGenerator::kPositionInput),
			  QStringLiteral("Position"));
	EXPECT_EQ(node.GetInputName(olive::MatrixGenerator::kAnchorInput),
			  QStringLiteral("Anchor Point"));

	const QStringList autoscale = node.GetComboBoxStrings(
		olive::TransformDistortNode::kAutoscaleInput);
	ASSERT_EQ(autoscale.size(), 4);
	EXPECT_EQ(autoscale.at(int(olive::TransformDistortNode::kAutoScaleNone)),
			  QStringLiteral("None"));
	EXPECT_EQ(autoscale.at(int(olive::TransformDistortNode::kAutoScaleFit)),
			  QStringLiteral("Fit"));
	EXPECT_EQ(autoscale.at(int(olive::TransformDistortNode::kAutoScaleFill)),
			  QStringLiteral("Fill"));
	EXPECT_EQ(autoscale.at(int(olive::TransformDistortNode::kAutoScaleStretch)),
			  QStringLiteral("Stretch"));

	const QStringList interpolation = node.GetComboBoxStrings(
		olive::TransformDistortNode::kInterpolationInput);
	ASSERT_EQ(interpolation.size(), 3);
	EXPECT_EQ(interpolation.at(int(olive::Texture::kNearest)),
			  QStringLiteral("Nearest Neighbor"));
	EXPECT_EQ(interpolation.at(int(olive::Texture::kLinear)),
			  QStringLiteral("Bilinear"));
	EXPECT_EQ(interpolation.at(int(olive::Texture::kMipmappedLinear)),
			  QStringLiteral("Mipmapped Bilinear"));
}

TEST(TransformDistortNode, GetShaderCodeReturnsEmptyCode)
{
	olive::TransformDistortNode node;

	// The transform is applied through the ove_mvpmat uniform of the default
	// shader, so the node provides no shader code of its own
	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_TRUE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsIdentityWhenMatching)
{
	// With identical sequence and texture resolutions, no offset and an
	// identity input matrix, the adjusted matrix must remain identity (the
	// scale to clip space and back cancels out)
	const QMatrix4x4 adjusted = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 1024.0f), QVector2D(1024.0f, 1024.0f),
		QVector2D(0.0f, 0.0f), olive::TransformDistortNode::kAutoScaleNone);
	EXPECT_TRUE(adjusted.isIdentity());
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsAppliesOffset)
{
	// The offset lives in texture pixel space: with matching 1024x1024
	// resolutions, offsetting by (128, 256) maps the origin to clip space
	// (128*2/1024, 256*2/1024) = (0.25, 0.5)
	const QMatrix4x4 adjusted = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 1024.0f), QVector2D(1024.0f, 1024.0f),
		QVector2D(128.0f, 256.0f), olive::TransformDistortNode::kAutoScaleNone);

	const QVector3D mapped = adjusted.map(QVector3D(0.0f, 0.0f, 0.0f));
	EXPECT_FLOAT_EQ(mapped.x(), 0.25f);
	EXPECT_FLOAT_EQ(mapped.y(), 0.5f);
	EXPECT_FLOAT_EQ(mapped.z(), 0.0f);
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsScalesToClipSpace)
{
	// Without auto-scale a 512x256 texture in a 1024x512 sequence covers only
	// half the frame in each axis
	const QMatrix4x4 adjusted = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 512.0f), QVector2D(512.0f, 256.0f),
		QVector2D(0.0f, 0.0f), olive::TransformDistortNode::kAutoScaleNone);

	const QVector3D corner = adjusted.map(QVector3D(1.0f, 1.0f, 0.0f));
	EXPECT_FLOAT_EQ(corner.x(), 0.5f);
	EXPECT_FLOAT_EQ(corner.y(), 0.5f);
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsStretchFillsSequence)
{
	// Stretch distorts the texture to the sequence aspect ratio exactly
	const QMatrix4x4 adjusted = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 512.0f), QVector2D(512.0f, 256.0f),
		QVector2D(0.0f, 0.0f), olive::TransformDistortNode::kAutoScaleStretch);

	const QVector3D corner = adjusted.map(QVector3D(1.0f, 1.0f, 0.0f));
	EXPECT_FLOAT_EQ(corner.x(), 1.0f);
	EXPECT_FLOAT_EQ(corner.y(), 1.0f);
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsFitWideFootage)
{
	// Footage wider than the sequence (AR 4.0 in AR 2.0) is scaled by width,
	// leaving letterbox bars: the vertical clip extent shrinks to 0.5
	const QMatrix4x4 fit = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 512.0f), QVector2D(1024.0f, 256.0f),
		QVector2D(0.0f, 0.0f), olive::TransformDistortNode::kAutoScaleFit);

	const QVector3D corner = fit.map(QVector3D(1.0f, 1.0f, 0.0f));
	EXPECT_FLOAT_EQ(corner.x(), 1.0f);
	EXPECT_FLOAT_EQ(corner.y(), 0.5f);
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsFillWideFootage)
{
	// Fill scales the same footage by height instead, cropping the sides
	const QMatrix4x4 fill = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 512.0f), QVector2D(1024.0f, 256.0f),
		QVector2D(0.0f, 0.0f), olive::TransformDistortNode::kAutoScaleFill);

	const QVector3D corner = fill.map(QVector3D(1.0f, 1.0f, 0.0f));
	EXPECT_FLOAT_EQ(corner.x(), 2.0f);
	EXPECT_FLOAT_EQ(corner.y(), 1.0f);
}

TEST(TransformDistortNode, AdjustMatrixByResolutionsFitTallFootage)
{
	// Footage narrower than the sequence (AR 0.5 in AR 2.0) is scaled by
	// height, leaving pillarbox bars
	const QMatrix4x4 fit = olive::TransformDistortNode::AdjustMatrixByResolutions(
		QMatrix4x4(), QVector2D(1024.0f, 512.0f), QVector2D(256.0f, 512.0f),
		QVector2D(0.0f, 0.0f), olive::TransformDistortNode::kAutoScaleFit);

	const QVector3D corner = fit.map(QVector3D(1.0f, 1.0f, 0.0f));
	EXPECT_FLOAT_EQ(corner.x(), 0.25f);
	EXPECT_FLOAT_EQ(corner.y(), 1.0f);
}

TEST(TransformDistortNode, ValueWithoutTexturePushesMatrixOnly)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TransformDistortNode>(&project);

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(1920, 1080));

	// The generated matrix is always pushed; with no texture connected the
	// re-pushed texture value is a null texture
	const olive::NodeValue matrix = table.Get(olive::NodeValue::kMatrix);
	ASSERT_EQ(int(matrix.type()), int(olive::NodeValue::kMatrix));
	EXPECT_TRUE(matrix.toMatrix().isIdentity());

	EXPECT_TRUE(table.Get(olive::NodeValue::kTexture).toTexture() == nullptr);
}

TEST(TransformDistortNode, ValueWithIdentityTransformPassesTextureThrough)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TransformDistortNode>(&project);
	auto *constant = AddNode<ConstantTextureNode>(&project);

	// Texture matching the sequence resolution with default transform values
	// produces an identity adjusted matrix, which the node treats as a no-op
	const olive::TexturePtr base = MakeDummyTexture(1024, 1024);
	constant->SetTexture(base);
	olive::Node::ConnectEdge(
		constant,
		olive::NodeInput(node, olive::TransformDistortNode::kTextureInput));

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(1024, 1024));

	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, base);
	EXPECT_FALSE(out->IsJob());

	EXPECT_TRUE(table.Get(olive::NodeValue::kMatrix).toMatrix().isIdentity());
}

TEST(TransformDistortNode, ValueWithTexturePushesMatrixJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TransformDistortNode>(&project);
	auto *constant = AddNode<ConstantTextureNode>(&project);

	// A 64x48 texture in a 1920x1080 sequence yields a non-identity matrix
	const olive::TexturePtr base = MakeDummyTexture(64, 48);
	constant->SetTexture(base);
	olive::Node::ConnectEdge(
		constant,
		olive::NodeInput(node, olive::TransformDistortNode::kTextureInput));

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(1920, 1080));

	olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	// The job adopts the sequence resolution, not the texture's, since the
	// transform may change the apparent size
	EXPECT_EQ(out->params().width(), 1920);
	EXPECT_EQ(out->params().height(), 1080);

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);

	// The original texture is fed in as ove_maintex
	EXPECT_EQ(job->Get(QStringLiteral("ove_maintex")).toTexture(), base);

	// The mvp matrix scales the texture into sequence clip space:
	// 64/1920 on X and 48/1080 on Y
	const QMatrix4x4 mvp = job->Get(QStringLiteral("ove_mvpmat")).toMatrix();
	EXPECT_NEAR(mvp(0, 0), 64.0 / 1920.0, 1e-6);
	EXPECT_NEAR(mvp(1, 1), 48.0 / 1080.0, 1e-6);
	EXPECT_FLOAT_EQ(mvp(2, 2), 1.0f);
	EXPECT_FLOAT_EQ(mvp(3, 3), 1.0f);

	// The raw generated matrix (identity here) is pushed alongside the job
	EXPECT_TRUE(table.Get(olive::NodeValue::kMatrix).toMatrix().isIdentity());

	// Interpolation defaults to mipmapped bilinear and follows the input
	EXPECT_EQ(int(job->GetInterpolation(QStringLiteral("ove_maintex"))),
			  int(olive::Texture::kMipmappedLinear));

	node->SetStandardValue(olive::TransformDistortNode::kInterpolationInput,
						   int(olive::Texture::kNearest));
	table = GenerateTable(node, SequenceParams(1920, 1080));
	out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(int(job->GetInterpolation(QStringLiteral("ove_maintex"))),
			  int(olive::Texture::kNearest));
}

TEST(TransformDistortNode, ValueBakesPositionIntoJobMatrix)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TransformDistortNode>(&project);
	node->SetStandardValue(olive::MatrixGenerator::kPositionInput,
						   QVector2D(100.0f, 50.0f));

	auto *constant = AddNode<ConstantTextureNode>(&project);
	constant->SetTexture(MakeDummyTexture(64, 48));
	olive::Node::ConnectEdge(
		constant,
		olive::NodeInput(node, olive::TransformDistortNode::kTextureInput));

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(1920, 1080));

	// The table matrix is the pure transform: a 100x50 pixel translation
	const QMatrix4x4 generated = table.Get(olive::NodeValue::kMatrix).toMatrix();
	const QVector3D raw = generated.map(QVector3D(0.0f, 0.0f, 0.0f));
	EXPECT_FLOAT_EQ(raw.x(), 100.0f);
	EXPECT_FLOAT_EQ(raw.y(), 50.0f);

	// The job matrix expresses the same translation in clip space
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);

	const QVector3D clip = job->Get(QStringLiteral("ove_mvpmat"))
							   .toMatrix()
							   .map(QVector3D(0.0f, 0.0f, 0.0f));
	EXPECT_NEAR(clip.x(), 100.0 * 2.0 / 1920.0, 1e-6);
	EXPECT_NEAR(clip.y(), 50.0 * 2.0 / 1080.0, 1e-6);
}

// -----------------------------------------------------------------------------
// CropDistortNode
// -----------------------------------------------------------------------------

TEST(CropDistortNode, MetadataIsCorrect)
{
	olive::CropDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.crop"));
	EXPECT_EQ(node.Name(), QStringLiteral("Crop"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::CropDistortNode::kTextureInput);
}

TEST(CropDistortNode, InputDefinitionsAndDefaults)
{
	olive::CropDistortNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::CropDistortNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::CropDistortNode::kTextureInput));

	// All four sides are 0..1 percentage sliders defaulting to zero
	const QString sides[] = { olive::CropDistortNode::kLeftInput,
							  olive::CropDistortNode::kTopInput,
							  olive::CropDistortNode::kRightInput,
							  olive::CropDistortNode::kBottomInput };
	for (const QString &side : sides) {
		EXPECT_EQ(int(node.GetInputDataType(side)), int(olive::NodeValue::kFloat));
		EXPECT_DOUBLE_EQ(node.GetStandardValue(side).toDouble(), 0.0);
		EXPECT_DOUBLE_EQ(
			node.GetInputProperty(side, QStringLiteral("min")).toDouble(), 0.0);
		EXPECT_DOUBLE_EQ(
			node.GetInputProperty(side, QStringLiteral("max")).toDouble(), 1.0);
		EXPECT_EQ(node.GetInputProperty(side, QStringLiteral("view")).toInt(),
				  int(olive::FloatSlider::kPercentage));
	}

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::CropDistortNode::kFeatherInput).toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::CropDistortNode::kFeatherInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);
}

TEST(CropDistortNode, RetranslateSetsInputNames)
{
	olive::CropDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::CropDistortNode::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::CropDistortNode::kLeftInput),
			  QStringLiteral("Left"));
	EXPECT_EQ(node.GetInputName(olive::CropDistortNode::kTopInput),
			  QStringLiteral("Top"));
	EXPECT_EQ(node.GetInputName(olive::CropDistortNode::kRightInput),
			  QStringLiteral("Right"));
	EXPECT_EQ(node.GetInputName(olive::CropDistortNode::kBottomInput),
			  QStringLiteral("Bottom"));
	EXPECT_EQ(node.GetInputName(olive::CropDistortNode::kFeatherInput),
			  QStringLiteral("Feather"));
}

TEST(CropDistortNode, GetShaderCodeLoadsCropShader)
{
	olive::CropDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("left_in")));
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("feather_in")));
	EXPECT_TRUE(code.vert_code().isEmpty());
}

TEST(CropDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::CropDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(CropDistortNode, ValueWithZeroCropPassesTextureThrough)
{
	olive::CropDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(120, 80);
	olive::NodeValueTable table;
	node.Value(MakeTextureRow(olive::CropDistortNode::kTextureInput, tex),
			   olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(CropDistortNode, ValueWithOnlyFeatherPassesTextureThrough)
{
	olive::CropDistortNode node;

	// NOTE: the node only checks the four crop sides when deciding to run the
	// shader; a feather without any crop is silently ignored
	const olive::TexturePtr tex = MakeDummyTexture(120, 80);
	olive::NodeValueRow row =
		MakeTextureRow(olive::CropDistortNode::kTextureInput, tex);
	row.insert(olive::CropDistortNode::kFeatherInput, FloatValue(5.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(CropDistortNode, ValueWithCropPushesShaderJob)
{
	olive::CropDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(120, 80);
	olive::NodeValueRow row =
		MakeTextureRow(olive::CropDistortNode::kTextureInput, tex);
	row.insert(olive::CropDistortNode::kLeftInput, FloatValue(0.25));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	// The job reuses the input texture's params
	EXPECT_EQ(out->params().width(), tex->params().width());
	EXPECT_EQ(out->params().height(), tex->params().height());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_DOUBLE_EQ(job->Get(olive::CropDistortNode::kLeftInput).toDouble(),
					 0.25);
	EXPECT_EQ(job->Get(QStringLiteral("resolution_in")).toVec2(),
			  QVector2D(120.0f, 80.0f));
}

// -----------------------------------------------------------------------------
// FlipDistortNode
// -----------------------------------------------------------------------------

TEST(FlipDistortNode, MetadataIsCorrect)
{
	olive::FlipDistortNode node;
	// NOTE: unlike most Olive nodes ("org.olivevideoeditor.Olive.*"), the
	// domain (inconsistent ID, documented here as a suspected bug)
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.flip"));
	EXPECT_EQ(node.Name(), QStringLiteral("Flip"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::FlipDistortNode::kTextureInput);
}

TEST(FlipDistortNode, InputDefaults)
{
	olive::FlipDistortNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::FlipDistortNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::FlipDistortNode::kTextureInput));

	EXPECT_EQ(
		int(node.GetInputDataType(olive::FlipDistortNode::kHorizontalInput)),
		int(olive::NodeValue::kBoolean));
	EXPECT_FALSE(node.GetStandardValue(olive::FlipDistortNode::kHorizontalInput)
					 .toBool());
	EXPECT_EQ(int(node.GetInputDataType(olive::FlipDistortNode::kVerticalInput)),
			  int(olive::NodeValue::kBoolean));
	EXPECT_FALSE(node.GetStandardValue(olive::FlipDistortNode::kVerticalInput)
					 .toBool());
}

TEST(FlipDistortNode, RetranslateSetsInputNames)
{
	olive::FlipDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::FlipDistortNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::FlipDistortNode::kHorizontalInput),
			  QStringLiteral("Horizontal"));
	EXPECT_EQ(node.GetInputName(olive::FlipDistortNode::kVerticalInput),
			  QStringLiteral("Vertical"));
}

TEST(FlipDistortNode, GetShaderCodeLoadsFlipShader)
{
	olive::FlipDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("horiz_in")));
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("vert_in")));
}

TEST(FlipDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::FlipDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(FlipDistortNode, ValueWithNoFlipPassesTextureThrough)
{
	olive::FlipDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueTable table;
	node.Value(MakeTextureRow(olive::FlipDistortNode::kTextureInput, tex),
			   olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(FlipDistortNode, ValueWithFlipPushesShaderJob)
{
	olive::FlipDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::FlipDistortNode::kTextureInput, tex);
	row.insert(olive::FlipDistortNode::kHorizontalInput, BoolValue(true));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	EXPECT_EQ(out->params().width(), tex->params().width());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_TRUE(job->Get(olive::FlipDistortNode::kHorizontalInput).toBool());
	EXPECT_FALSE(job->Get(olive::FlipDistortNode::kVerticalInput).toBool());

	// Vertical flip on its own also triggers the shader
	row.insert(olive::FlipDistortNode::kHorizontalInput, BoolValue(false));
	row.insert(olive::FlipDistortNode::kVerticalInput, BoolValue(true));

	olive::NodeValueTable vertical_table;
	node.Value(row, olive::NodeGlobals(), &vertical_table);

	ASSERT_EQ(vertical_table.Count(), 1);
	out = GetOutputTexture(vertical_table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_FALSE(job->Get(olive::FlipDistortNode::kHorizontalInput).toBool());
	EXPECT_TRUE(job->Get(olive::FlipDistortNode::kVerticalInput).toBool());
}

// -----------------------------------------------------------------------------
// CornerPinDistortNode
// -----------------------------------------------------------------------------

TEST(CornerPinDistortNode, MetadataIsCorrect)
{
	olive::CornerPinDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.cornerpin"));
	EXPECT_EQ(node.Name(), QStringLiteral("Corner Pin"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::CornerPinDistortNode::kTextureInput);
}

TEST(CornerPinDistortNode, InputDefaults)
{
	olive::CornerPinDistortNode node;

	EXPECT_EQ(
		int(node.GetInputDataType(olive::CornerPinDistortNode::kTextureInput)),
		int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::CornerPinDistortNode::kTextureInput));

	EXPECT_EQ(int(node.GetInputDataType(
				  olive::CornerPinDistortNode::kPerspectiveInput)),
			  int(olive::NodeValue::kBoolean));
	EXPECT_TRUE(node.GetStandardValue(
					olive::CornerPinDistortNode::kPerspectiveInput)
					.toBool());

	// All four corners are pixel offsets relative to their respective image
	// corner and default to no offset
	const QString corners[] = { olive::CornerPinDistortNode::kTopLeftInput,
								olive::CornerPinDistortNode::kTopRightInput,
								olive::CornerPinDistortNode::kBottomRightInput,
								olive::CornerPinDistortNode::kBottomLeftInput };
	for (const QString &corner : corners) {
		EXPECT_EQ(int(node.GetInputDataType(corner)),
				  int(olive::NodeValue::kVec2));
		EXPECT_EQ(node.GetStandardValue(corner).value<QVector2D>(),
				  QVector2D(0.0f, 0.0f));
	}
}

TEST(CornerPinDistortNode, RetranslateSetsInputNames)
{
	olive::CornerPinDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::CornerPinDistortNode::kTextureInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::CornerPinDistortNode::kPerspectiveInput),
			  QStringLiteral("Perspective"));
	EXPECT_EQ(node.GetInputName(olive::CornerPinDistortNode::kTopLeftInput),
			  QStringLiteral("Top Left"));
	EXPECT_EQ(node.GetInputName(olive::CornerPinDistortNode::kTopRightInput),
			  QStringLiteral("Top Right"));
	EXPECT_EQ(node.GetInputName(olive::CornerPinDistortNode::kBottomRightInput),
			  QStringLiteral("Bottom Right"));
	EXPECT_EQ(node.GetInputName(olive::CornerPinDistortNode::kBottomLeftInput),
			  QStringLiteral("Bottom Left"));
}

TEST(CornerPinDistortNode, GetShaderCodeLoadsFragAndVertShaders)
{
	olive::CornerPinDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("perspective_in")));
	// Corner Pin is one of the few nodes with a custom vertex shader
	EXPECT_FALSE(code.vert_code().isEmpty());
	EXPECT_TRUE(code.vert_code().contains(QStringLiteral("top_left_in")));
}

TEST(CornerPinDistortNode, ValueToPixelConvertsOffsetsToPixels)
{
	olive::CornerPinDistortNode node;

	olive::NodeValueRow row;
	row.insert(olive::CornerPinDistortNode::kTopLeftInput,
			   Vec2Value(QVector2D(10.0f, 20.0f)));
	row.insert(olive::CornerPinDistortNode::kTopRightInput,
			   Vec2Value(QVector2D(-30.0f, 40.0f)));
	row.insert(olive::CornerPinDistortNode::kBottomRightInput,
			   Vec2Value(QVector2D(-50.0f, -60.0f)));
	row.insert(olive::CornerPinDistortNode::kBottomLeftInput,
			   Vec2Value(QVector2D(70.0f, -80.0f)));

	const QVector2D resolution(200.0f, 100.0f);

	// Top-left offsets are relative to (0, 0)
	const QPointF top_left = node.ValueToPixel(0, row, resolution);
	EXPECT_DOUBLE_EQ(top_left.x(), 10.0);
	EXPECT_DOUBLE_EQ(top_left.y(), 20.0);

	// Top-right offsets are relative to (width, 0)
	const QPointF top_right = node.ValueToPixel(1, row, resolution);
	EXPECT_DOUBLE_EQ(top_right.x(), 170.0);
	EXPECT_DOUBLE_EQ(top_right.y(), 40.0);

	// Bottom-right offsets are relative to (width, height)
	const QPointF bottom_right = node.ValueToPixel(2, row, resolution);
	EXPECT_DOUBLE_EQ(bottom_right.x(), 150.0);
	EXPECT_DOUBLE_EQ(bottom_right.y(), 40.0);

	// Bottom-left offsets are relative to (0, height)
	const QPointF bottom_left = node.ValueToPixel(3, row, resolution);
	EXPECT_DOUBLE_EQ(bottom_left.x(), 70.0);
	EXPECT_DOUBLE_EQ(bottom_left.y(), 20.0);
}

TEST(CornerPinDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::CornerPinDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(CornerPinDistortNode, ValueWithDefaultCornersPassesTextureThrough)
{
	olive::CornerPinDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(100, 100);
	olive::NodeValueTable table;
	node.Value(MakeTextureRow(olive::CornerPinDistortNode::kTextureInput, tex),
			   olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(CornerPinDistortNode, ValueWithMovedCornerPushesVertexCoordinates)
{
	olive::CornerPinDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(100, 100);
	olive::NodeValueRow row =
		MakeTextureRow(olive::CornerPinDistortNode::kTextureInput, tex);
	row.insert(olive::CornerPinDistortNode::kTopLeftInput,
			   Vec2Value(QVector2D(10.0f, 20.0f)));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	EXPECT_EQ(out->params().width(), tex->params().width());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(QStringLiteral("resolution_in")).toVec2(),
			  QVector2D(100.0f, 100.0f));

	// Slider offsets are converted to pixel positions and then to clip space
	// (-1..1): top-left (10, 20) -> (-0.8, -0.6), the untouched corners land
	// on the default quad
	const QVector<float> &vertices = job->GetVertexCoordinates();
	ASSERT_EQ(int(vertices.size()), 18);

	// First triangle
	EXPECT_FLOAT_EQ(vertices.at(0), -0.8f);
	EXPECT_FLOAT_EQ(vertices.at(1), -0.6f);
	EXPECT_FLOAT_EQ(vertices.at(2), 0.0f);
	EXPECT_FLOAT_EQ(vertices.at(3), 1.0f);
	EXPECT_FLOAT_EQ(vertices.at(4), -1.0f);
	EXPECT_FLOAT_EQ(vertices.at(5), 0.0f);
	EXPECT_FLOAT_EQ(vertices.at(6), 1.0f);
	EXPECT_FLOAT_EQ(vertices.at(7), 1.0f);
	EXPECT_FLOAT_EQ(vertices.at(8), 0.0f);

	// Second triangle
	EXPECT_FLOAT_EQ(vertices.at(9), -0.8f);
	EXPECT_FLOAT_EQ(vertices.at(10), -0.6f);
	EXPECT_FLOAT_EQ(vertices.at(12), -1.0f);
	EXPECT_FLOAT_EQ(vertices.at(13), 1.0f);
	EXPECT_FLOAT_EQ(vertices.at(15), 1.0f);
	EXPECT_FLOAT_EQ(vertices.at(16), 1.0f);
}

// -----------------------------------------------------------------------------
// MaskDistortNode
// -----------------------------------------------------------------------------

TEST(MaskDistortNode, MetadataIsCorrect)
{
	olive::MaskDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.mask"));
	EXPECT_EQ(node.Name(), QStringLiteral("Mask"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	// From GeneratorWithMerge: the base texture is the effect input
	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::GeneratorWithMerge::kBaseInput);
}

TEST(MaskDistortNode, InputDefinitionsAndDefaults)
{
	olive::MaskDistortNode node;

	EXPECT_EQ(
		int(node.GetInputDataType(olive::MaskDistortNode::kInvertInput)),
		int(olive::NodeValue::kBoolean));
	EXPECT_FALSE(node.GetStandardValue(olive::MaskDistortNode::kInvertInput)
					 .toBool());

	EXPECT_EQ(
		int(node.GetInputDataType(olive::MaskDistortNode::kFeatherInput)),
		int(olive::NodeValue::kFloat));
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::MaskDistortNode::kFeatherInput).toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::MaskDistortNode::kFeatherInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);

	// From PolygonGenerator: the color is hidden because the mask must stay
	// white for the multiply to work, and the shape defaults to a pentagon
	EXPECT_TRUE(node.IsInputHidden(olive::PolygonGenerator::kColorInput));
	EXPECT_TRUE(node.InputIsArray(olive::PolygonGenerator::kPointsInput));
	EXPECT_EQ(node.InputArraySize(olive::PolygonGenerator::kPointsInput), 5);
}

TEST(MaskDistortNode, RetranslateSetsInputNames)
{
	olive::MaskDistortNode node;
	node.Retranslate();

	// The base input is renamed from GeneratorWithMerge's "Base"
	EXPECT_EQ(node.GetInputName(olive::GeneratorWithMerge::kBaseInput),
			  QStringLiteral("Texture"));
	EXPECT_EQ(node.GetInputName(olive::MaskDistortNode::kInvertInput),
			  QStringLiteral("Invert"));
	EXPECT_EQ(node.GetInputName(olive::MaskDistortNode::kFeatherInput),
			  QStringLiteral("Feather"));

	// Inherited names from PolygonGenerator
	EXPECT_EQ(node.GetInputName(olive::PolygonGenerator::kPointsInput),
			  QStringLiteral("Points"));
	EXPECT_EQ(node.GetInputName(olive::PolygonGenerator::kColorInput),
			  QStringLiteral("Color"));
}

TEST(MaskDistortNode, GetShaderCodeSelectsShaderByRequestId)
{
	olive::MaskDistortNode node;

	const olive::ShaderCode merge = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("mrg")));
	EXPECT_FALSE(merge.frag_code().isEmpty());
	EXPECT_TRUE(merge.frag_code().contains(QStringLiteral("tex_a")));

	const olive::ShaderCode feather = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("feather")));
	EXPECT_FALSE(feather.frag_code().isEmpty());
	EXPECT_TRUE(feather.frag_code().contains(QStringLiteral("radius_in")));

	const olive::ShaderCode invert = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("invert")));
	EXPECT_FALSE(invert.frag_code().isEmpty());
	EXPECT_TRUE(invert.frag_code().contains(QStringLiteral("tex_in")));

	// Unknown ids fall through to PolygonGenerator, which serves "rgb"
	const olive::ShaderCode rgb = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("rgb")));
	EXPECT_FALSE(rgb.frag_code().isEmpty());
	EXPECT_TRUE(rgb.frag_code().contains(QStringLiteral("texture_in")));

	const olive::ShaderCode unknown = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("bogus")));
	EXPECT_TRUE(unknown.frag_code().isEmpty());
	EXPECT_TRUE(unknown.vert_code().isEmpty());
}

TEST(MaskDistortNode, ValueWithoutTexturePushesGeneratePipeline)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MaskDistortNode>(&project);

	const olive::VideoParams vparams = SequenceParams(320, 240);
	olive::NodeValueTable table = GenerateTable(node, vparams);

	// Without a base the mask still generates its polygon, wrapped in the
	// "rgb" conversion job
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	EXPECT_EQ(out->params().width(), vparams.width());
	EXPECT_EQ(out->params().height(), vparams.height());

	auto *rgb = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(rgb);
	EXPECT_EQ(rgb->GetShaderID(), QStringLiteral("rgb"));

	// The polygon color is forced to white
	const olive::core::Color color =
		rgb->Get(QStringLiteral("color_in")).toColor();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 1.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	// The nested generation job renders to an 8-bit buffer
	const olive::TexturePtr generate =
		rgb->Get(QStringLiteral("texture_in")).toTexture();
	ASSERT_TRUE(generate);
	ASSERT_TRUE(generate->IsJob());
	EXPECT_EQ(int(generate->params().format()), int(olive::core::PixelFormat::U8));
	EXPECT_TRUE(dynamic_cast<olive::GenerateJob *>(generate->job()));
}

TEST(MaskDistortNode, ValueWithInvertWrapsGenerationInInvertJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MaskDistortNode>(&project);
	node->SetStandardValue(olive::MaskDistortNode::kInvertInput, true);

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(320, 240));

	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *invert = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(invert);
	EXPECT_EQ(invert->GetShaderID(), QStringLiteral("invert"));

	// The inverted texture is the usual rgb generation pipeline
	const olive::TexturePtr rgb_tex =
		invert->Get(QStringLiteral("tex_in")).toTexture();
	ASSERT_TRUE(rgb_tex);
	ASSERT_TRUE(rgb_tex->IsJob());
	auto *rgb = dynamic_cast<olive::ShaderJob *>(rgb_tex->job());
	ASSERT_TRUE(rgb);
	EXPECT_EQ(rgb->GetShaderID(), QStringLiteral("rgb"));
}

TEST(MaskDistortNode, ValueWithTexturePushesMergeJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MaskDistortNode>(&project);
	auto *constant = AddNode<ConstantTextureNode>(&project);

	const olive::TexturePtr base = MakeDummyTexture(64, 48);
	constant->SetTexture(base);
	olive::Node::ConnectEdge(
		constant, olive::NodeInput(node, olive::GeneratorWithMerge::kBaseInput));

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(320, 240));

	// With a base the mask multiplies it by the generated polygon
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	EXPECT_EQ(out->params().width(), base->params().width());
	EXPECT_EQ(out->params().height(), base->params().height());

	auto *merge = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(merge);
	EXPECT_EQ(merge->GetShaderID(), QStringLiteral("mrg"));
	EXPECT_EQ(merge->Get(QStringLiteral("tex_a")).toTexture(), base);

	// Without a feather, tex_b is the rgb generation pipeline directly
	const olive::TexturePtr tex_b =
		merge->Get(QStringLiteral("tex_b")).toTexture();
	ASSERT_TRUE(tex_b);
	ASSERT_TRUE(tex_b->IsJob());
	auto *rgb = dynamic_cast<olive::ShaderJob *>(tex_b->job());
	ASSERT_TRUE(rgb);
	EXPECT_EQ(rgb->GetShaderID(), QStringLiteral("rgb"));
}

TEST(MaskDistortNode, ValueWithFeatherNestsBlurJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MaskDistortNode>(&project);
	node->SetStandardValue(olive::MaskDistortNode::kFeatherInput, 10.0);

	auto *constant = AddNode<ConstantTextureNode>(&project);
	const olive::TexturePtr base = MakeDummyTexture(64, 48);
	constant->SetTexture(base);
	olive::Node::ConnectEdge(
		constant, olive::NodeInput(node, olive::GeneratorWithMerge::kBaseInput));

	olive::NodeValueTable table = GenerateTable(node, SequenceParams(320, 240));

	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *merge = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(merge);
	EXPECT_EQ(merge->GetShaderID(), QStringLiteral("mrg"));

	// With a feather, tex_b becomes a two-iteration gaussian blur of the mask
	const olive::TexturePtr tex_b =
		merge->Get(QStringLiteral("tex_b")).toTexture();
	ASSERT_TRUE(tex_b);
	ASSERT_TRUE(tex_b->IsJob());

	auto *feather = dynamic_cast<olive::ShaderJob *>(tex_b->job());
	ASSERT_TRUE(feather);
	EXPECT_EQ(feather->GetShaderID(), QStringLiteral("feather"));
	EXPECT_EQ(feather->GetIterationCount(), 2);
	EXPECT_EQ(feather->GetIterativeInput(), olive::BlurFilterNode::kTextureInput);
	EXPECT_DOUBLE_EQ(
		feather->Get(olive::BlurFilterNode::kRadiusInput).toDouble(), 10.0);
	EXPECT_EQ(feather->Get(olive::BlurFilterNode::kMethodInput).toInt(),
			  int(olive::BlurFilterNode::kGaussian));
	EXPECT_EQ(feather->Get(QStringLiteral("resolution_in")).toVec2(),
			  base->virtual_resolution());
}

// -----------------------------------------------------------------------------
// RippleDistortNode
// -----------------------------------------------------------------------------

TEST(RippleDistortNode, MetadataIsCorrect)
{
	olive::RippleDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.ripple"));
	EXPECT_EQ(node.Name(), QStringLiteral("Ripple"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::RippleDistortNode::kTextureInput);
}

TEST(RippleDistortNode, InputDefaults)
{
	olive::RippleDistortNode node;

	EXPECT_EQ(
		int(node.GetInputDataType(olive::RippleDistortNode::kTextureInput)),
		int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::RippleDistortNode::kTextureInput));

	EXPECT_DOUBLE_EQ(node.GetStandardValue(olive::RippleDistortNode::kEvolutionInput)
						 .toDouble(),
					 0.0);
	EXPECT_DOUBLE_EQ(node.GetStandardValue(olive::RippleDistortNode::kIntensityInput)
						 .toDouble(),
					 100.0);
	EXPECT_DOUBLE_EQ(node.GetStandardValue(olive::RippleDistortNode::kFrequencyInput)
						 .toDouble(),
					 1.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::RippleDistortNode::kFrequencyInput,
										   QStringLiteral("base"))
						 .toDouble(),
					 0.01);
	EXPECT_EQ(node.GetStandardValue(olive::RippleDistortNode::kPositionInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));
	EXPECT_FALSE(node.GetStandardValue(olive::RippleDistortNode::kStretchInput)
					 .toBool());
}

TEST(RippleDistortNode, RetranslateSetsInputNames)
{
	olive::RippleDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::RippleDistortNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::RippleDistortNode::kFrequencyInput),
			  QStringLiteral("Frequency"));
	EXPECT_EQ(node.GetInputName(olive::RippleDistortNode::kIntensityInput),
			  QStringLiteral("Intensity"));
	EXPECT_EQ(node.GetInputName(olive::RippleDistortNode::kEvolutionInput),
			  QStringLiteral("Evolution"));
	EXPECT_EQ(node.GetInputName(olive::RippleDistortNode::kPositionInput),
			  QStringLiteral("Position"));
	EXPECT_EQ(node.GetInputName(olive::RippleDistortNode::kStretchInput),
			  QStringLiteral("Stretch"));
}

TEST(RippleDistortNode, GetShaderCodeLoadsRippleShader)
{
	olive::RippleDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("evolution_in")));
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("intensity_in")));
}

TEST(RippleDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::RippleDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(RippleDistortNode, ValueWithZeroIntensityPassesTextureThrough)
{
	olive::RippleDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::RippleDistortNode::kTextureInput, tex);
	row.insert(olive::RippleDistortNode::kIntensityInput, FloatValue(0.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(RippleDistortNode, ValueWithIntensityPushesShaderJob)
{
	olive::RippleDistortNode node;

	// With a non-zero intensity the shader runs and receives the texture's
	// virtual resolution
	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::RippleDistortNode::kTextureInput, tex);
	row.insert(olive::RippleDistortNode::kIntensityInput, FloatValue(100.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());
	EXPECT_EQ(out->params().width(), tex->params().width());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_DOUBLE_EQ(job->Get(olive::RippleDistortNode::kIntensityInput).toDouble(),
					 100.0);
	EXPECT_EQ(job->Get(QStringLiteral("resolution_in")).toVec2(),
			  tex->virtual_resolution());
}

// -----------------------------------------------------------------------------
// SwirlDistortNode
// -----------------------------------------------------------------------------

TEST(SwirlDistortNode, MetadataIsCorrect)
{
	olive::SwirlDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.swirl"));
	EXPECT_EQ(node.Name(), QStringLiteral("Swirl"));
	EXPECT_EQ(node.Description(),
			  QStringLiteral("Distorts an image by swirling it around a center point."));
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::SwirlDistortNode::kTextureInput);
}

TEST(SwirlDistortNode, InputDefaults)
{
	olive::SwirlDistortNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::SwirlDistortNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::SwirlDistortNode::kTextureInput));

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::SwirlDistortNode::kRadiusInput).toDouble(),
		200.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::SwirlDistortNode::kRadiusInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::SwirlDistortNode::kAngleInput).toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::SwirlDistortNode::kAngleInput,
										   QStringLiteral("base"))
						 .toDouble(),
					 0.1);
	EXPECT_EQ(node.GetStandardValue(olive::SwirlDistortNode::kPositionInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));
}

TEST(SwirlDistortNode, RetranslateSetsInputNames)
{
	olive::SwirlDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::SwirlDistortNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::SwirlDistortNode::kRadiusInput),
			  QStringLiteral("Radius"));
	EXPECT_EQ(node.GetInputName(olive::SwirlDistortNode::kAngleInput),
			  QStringLiteral("Angle"));
	EXPECT_EQ(node.GetInputName(olive::SwirlDistortNode::kPositionInput),
			  QStringLiteral("Position"));
}

TEST(SwirlDistortNode, GetShaderCodeLoadsSwirlShader)
{
	olive::SwirlDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("radius_in")));
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("angle_in")));
}

TEST(SwirlDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::SwirlDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(SwirlDistortNode, ValueWithZeroAngleOrRadiusPassesTextureThrough)
{
	olive::SwirlDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(64, 48);

	// Zero angle neutralizes the swirl
	olive::NodeValueRow row =
		MakeTextureRow(olive::SwirlDistortNode::kTextureInput, tex);
	row.insert(olive::SwirlDistortNode::kAngleInput, FloatValue(0.0));
	row.insert(olive::SwirlDistortNode::kRadiusInput, FloatValue(200.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	EXPECT_EQ(GetOutputTexture(table), tex);

	// So does a zero radius
	row.insert(olive::SwirlDistortNode::kAngleInput, FloatValue(10.0));
	row.insert(olive::SwirlDistortNode::kRadiusInput, FloatValue(0.0));

	olive::NodeValueTable zero_radius_table;
	node.Value(row, olive::NodeGlobals(), &zero_radius_table);

	ASSERT_EQ(zero_radius_table.Count(), 1);
	EXPECT_EQ(GetOutputTexture(zero_radius_table), tex);
}

TEST(SwirlDistortNode, ValueWithAngleAndRadiusPushesShaderJob)
{
	olive::SwirlDistortNode node;

	// With a non-zero angle and radius the shader runs and receives the
	// texture's virtual resolution
	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::SwirlDistortNode::kTextureInput, tex);
	row.insert(olive::SwirlDistortNode::kAngleInput, FloatValue(10.0));
	row.insert(olive::SwirlDistortNode::kRadiusInput, FloatValue(200.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_DOUBLE_EQ(job->Get(olive::SwirlDistortNode::kAngleInput).toDouble(),
					 10.0);
	EXPECT_EQ(job->Get(QStringLiteral("resolution_in")).toVec2(),
			  tex->virtual_resolution());
}

// -----------------------------------------------------------------------------
// TileDistortNode
// -----------------------------------------------------------------------------

TEST(TileDistortNode, MetadataIsCorrect)
{
	olive::TileDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.tile"));
	EXPECT_EQ(node.Name(), QStringLiteral("Tile"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::TileDistortNode::kTextureInput);
}

TEST(TileDistortNode, InputDefaults)
{
	olive::TileDistortNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::TileDistortNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::TileDistortNode::kTextureInput));

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::TileDistortNode::kScaleInput).toDouble(),
		0.5);
	EXPECT_DOUBLE_EQ(node.GetInputProperty(olive::TileDistortNode::kScaleInput,
										   QStringLiteral("min"))
						 .toDouble(),
					 0.0);
	EXPECT_EQ(node.GetInputProperty(olive::TileDistortNode::kScaleInput,
									QStringLiteral("view"))
				  .toInt(),
			  int(olive::FloatSlider::kPercentage));

	EXPECT_EQ(node.GetStandardValue(olive::TileDistortNode::kPositionInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));

	EXPECT_EQ(int(node.GetInputDataType(olive::TileDistortNode::kAnchorInput)),
			  int(olive::NodeValue::kCombo));
	// The Anchor enum is private; 4 is kMiddleCenter
	EXPECT_EQ(node.GetStandardValue(olive::TileDistortNode::kAnchorInput).toInt(),
			  4);

	EXPECT_FALSE(node.GetStandardValue(olive::TileDistortNode::kMirrorXInput)
					 .toBool());
	EXPECT_FALSE(node.GetStandardValue(olive::TileDistortNode::kMirrorYInput)
					 .toBool());
}

TEST(TileDistortNode, RetranslateSetsNamesAndAnchorComboStrings)
{
	olive::TileDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::TileDistortNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::TileDistortNode::kScaleInput),
			  QStringLiteral("Scale"));
	EXPECT_EQ(node.GetInputName(olive::TileDistortNode::kPositionInput),
			  QStringLiteral("Position"));
	EXPECT_EQ(node.GetInputName(olive::TileDistortNode::kAnchorInput),
			  QStringLiteral("Anchor"));
	EXPECT_EQ(node.GetInputName(olive::TileDistortNode::kMirrorXInput),
			  QStringLiteral("Mirror Horizontally"));
	EXPECT_EQ(node.GetInputName(olive::TileDistortNode::kMirrorYInput),
			  QStringLiteral("Mirror Vertically"));

	const QStringList anchors =
		node.GetComboBoxStrings(olive::TileDistortNode::kAnchorInput);
	ASSERT_EQ(anchors.size(), 9);
	EXPECT_EQ(anchors.at(0), QStringLiteral("Top-Left"));
	EXPECT_EQ(anchors.at(1), QStringLiteral("Top-Center"));
	EXPECT_EQ(anchors.at(2), QStringLiteral("Top-Right"));
	EXPECT_EQ(anchors.at(3), QStringLiteral("Middle-Left"));
	EXPECT_EQ(anchors.at(4), QStringLiteral("Middle-Center"));
	EXPECT_EQ(anchors.at(5), QStringLiteral("Middle-Right"));
	EXPECT_EQ(anchors.at(6), QStringLiteral("Bottom-Left"));
	EXPECT_EQ(anchors.at(7), QStringLiteral("Bottom-Center"));
	EXPECT_EQ(anchors.at(8), QStringLiteral("Bottom-Right"));
}

TEST(TileDistortNode, GetShaderCodeLoadsTileShader)
{
	olive::TileDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("mirrorx_in")));
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("anchor_in")));
}

TEST(TileDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::TileDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(TileDistortNode, ValueWithUnitScalePassesTextureThrough)
{
	olive::TileDistortNode node;

	// A scale of exactly 1.0 is a no-op
	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::TileDistortNode::kTextureInput, tex);
	row.insert(olive::TileDistortNode::kScaleInput, FloatValue(1.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(TileDistortNode, ValueWithNonUnitScalePushesShaderJob)
{
	olive::TileDistortNode node;

	// Any scale other than 1.0 runs the shader
	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::TileDistortNode::kTextureInput, tex);
	row.insert(olive::TileDistortNode::kScaleInput, FloatValue(0.5));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_DOUBLE_EQ(job->Get(olive::TileDistortNode::kScaleInput).toDouble(),
					 0.5);
	EXPECT_EQ(job->Get(QStringLiteral("resolution_in")).toVec2(),
			  tex->virtual_resolution());
}

// -----------------------------------------------------------------------------
// WaveDistortNode
// -----------------------------------------------------------------------------

TEST(WaveDistortNode, MetadataIsCorrect)
{
	olive::WaveDistortNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.wave"));
	EXPECT_EQ(node.Name(), QStringLiteral("Wave"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryDistort));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::WaveDistortNode::kTextureInput);
}

TEST(WaveDistortNode, InputDefaults)
{
	olive::WaveDistortNode node;

	EXPECT_EQ(int(node.GetInputDataType(olive::WaveDistortNode::kTextureInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node.IsInputKeyframable(olive::WaveDistortNode::kTextureInput));

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::WaveDistortNode::kFrequencyInput).toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::WaveDistortNode::kIntensityInput).toDouble(),
		10.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::WaveDistortNode::kEvolutionInput).toDouble(),
		0.0);

	EXPECT_EQ(int(node.GetInputDataType(olive::WaveDistortNode::kVerticalInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node.GetStandardValue(olive::WaveDistortNode::kVerticalInput).toInt(),
			  0);
}

TEST(WaveDistortNode, RetranslateSetsNamesAndComboStrings)
{
	olive::WaveDistortNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::WaveDistortNode::kTextureInput),
			  QStringLiteral("Input"));
	EXPECT_EQ(node.GetInputName(olive::WaveDistortNode::kFrequencyInput),
			  QStringLiteral("Frequency"));
	EXPECT_EQ(node.GetInputName(olive::WaveDistortNode::kIntensityInput),
			  QStringLiteral("Intensity"));
	EXPECT_EQ(node.GetInputName(olive::WaveDistortNode::kEvolutionInput),
			  QStringLiteral("Evolution"));
	EXPECT_EQ(node.GetInputName(olive::WaveDistortNode::kVerticalInput),
			  QStringLiteral("Direction"));

	const QStringList directions =
		node.GetComboBoxStrings(olive::WaveDistortNode::kVerticalInput);
	ASSERT_EQ(directions.size(), 2);
	EXPECT_EQ(directions.at(0), QStringLiteral("Horizontal"));
	EXPECT_EQ(directions.at(1), QStringLiteral("Vertical"));
}

TEST(WaveDistortNode, GetShaderCodeLoadsWaveShader)
{
	olive::WaveDistortNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("vertical_in")));
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("intensity_in")));
}

TEST(WaveDistortNode, ValueWithoutTexturePushesNothing)
{
	olive::WaveDistortNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(WaveDistortNode, ValueWithZeroIntensityPassesTextureThrough)
{
	olive::WaveDistortNode node;

	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::WaveDistortNode::kTextureInput, tex);
	row.insert(olive::WaveDistortNode::kIntensityInput, FloatValue(0.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	EXPECT_EQ(out, tex);
	EXPECT_FALSE(out->IsJob());
}

TEST(WaveDistortNode, ValueWithIntensityPushesShaderJob)
{
	olive::WaveDistortNode node;

	// With a non-zero intensity the shader runs
	const olive::TexturePtr tex = MakeDummyTexture(64, 48);
	olive::NodeValueRow row =
		MakeTextureRow(olive::WaveDistortNode::kTextureInput, tex);
	row.insert(olive::WaveDistortNode::kIntensityInput, FloatValue(10.0));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	ASSERT_EQ(table.Count(), 1);
	const olive::TexturePtr out = GetOutputTexture(table);
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	// Unlike the other distorters, WaveDistortNode does not insert a
	// resolution_in; the job simply carries the row values with the input
	// texture's params
	EXPECT_EQ(out->params().width(), tex->params().width());
	EXPECT_EQ(out->params().height(), tex->params().height());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_DOUBLE_EQ(job->Get(olive::WaveDistortNode::kIntensityInput).toDouble(),
					 10.0);
	EXPECT_TRUE(job->Get(QStringLiteral("resolution_in")).toVec2().isNull());
}
