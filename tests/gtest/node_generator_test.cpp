#include <gtest/gtest.h>

#include <QMatrix4x4>
#include <QStringList>
#include <QVector2D>
#include <QVector3D>

#include "node/color/colormanager/colormanager.h"
#include "node/generator/matrix/matrix.h"
#include "node/generator/noise/noise.h"
#include "node/generator/polygon/polygon.h"
#include "node/generator/shape/shapenode.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv3.h"
#include "node/math/merge/merge.h"
#include "node/project.h"
#include "node/traverser.h"
#include "olive/core/util/color.h"
#include "render/job/generatejob.h"
#include "render/job/shaderjob.h"
#include "render/texture.h"
#include "widget/slider/floatslider.h"

namespace
{

// Node that pushes a fixed dummy texture, used to feed the base input of
// merge-capable generators without any renderer.
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
		return QStringLiteral("org.oak.test.constant_texture");
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

olive::VideoParams TestVideoParams()
{
	return olive::VideoParams(320, 240, olive::core::PixelFormat::U8, 4);
}

// A fresh traverser per call: NodeTraverser caches tables per node/range, so
// reusing one would return stale results after changing standard values.
olive::NodeValueTable GenerateTable(const olive::Node *node)
{
	olive::NodeTraverser traverser;
	return traverser.GenerateTable(node, FirstFrame());
}

olive::NodeValueTable GenerateTable(const olive::Node *node,
									const olive::VideoParams &vparams,
									const olive::TimeRange &range)
{
	olive::NodeTraverser traverser;
	traverser.SetCacheVideoParams(vparams);
	return traverser.GenerateTable(node, range);
}

olive::NodeValueTable GenerateTable(const olive::Node *node,
									const olive::VideoParams &vparams)
{
	return GenerateTable(node, vparams, FirstFrame());
}

olive::TexturePtr GetOutputTexture(const olive::NodeValueTable &table)
{
	return table.Get(olive::NodeValue::kTexture).toTexture();
}

} // namespace

TEST(MatrixGenerator, MetadataIsCorrect)
{
	olive::MatrixGenerator node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.ortho"));
	EXPECT_EQ(node.Name(), QStringLiteral("Orthographic Matrix"));
	EXPECT_EQ(node.ShortName(), QStringLiteral("Ortho"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryMath));
}

TEST(MatrixGenerator, InputDefaultsAndProperties)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);

	ASSERT_TRUE(node->HasInputWithID(olive::MatrixGenerator::kPositionInput));
	ASSERT_TRUE(node->HasInputWithID(olive::MatrixGenerator::kRotationInput));
	ASSERT_TRUE(node->HasInputWithID(olive::MatrixGenerator::kScaleInput));
	ASSERT_TRUE(
		node->HasInputWithID(olive::MatrixGenerator::kUniformScaleInput));
	ASSERT_TRUE(node->HasInputWithID(olive::MatrixGenerator::kAnchorInput));

	EXPECT_EQ(int(node->GetInputDataType(olive::MatrixGenerator::kPositionInput)),
			  int(olive::NodeValue::kVec2));
	EXPECT_EQ(int(node->GetInputDataType(olive::MatrixGenerator::kRotationInput)),
			  int(olive::NodeValue::kFloat));
	EXPECT_EQ(int(node->GetInputDataType(olive::MatrixGenerator::kScaleInput)),
			  int(olive::NodeValue::kVec2));
	EXPECT_EQ(
		int(node->GetInputDataType(olive::MatrixGenerator::kUniformScaleInput)),
		int(olive::NodeValue::kBoolean));
	EXPECT_EQ(int(node->GetInputDataType(olive::MatrixGenerator::kAnchorInput)),
			  int(olive::NodeValue::kVec2));

	EXPECT_EQ(node->GetStandardValue(olive::MatrixGenerator::kPositionInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));
	EXPECT_EQ(node->GetStandardValue(olive::MatrixGenerator::kRotationInput)
				  .toDouble(),
			  0.0);
	EXPECT_EQ(
		node->GetStandardValue(olive::MatrixGenerator::kScaleInput).value<QVector2D>(),
		QVector2D(1.0f, 1.0f));
	EXPECT_TRUE(node->GetStandardValue(olive::MatrixGenerator::kUniformScaleInput)
					.toBool());
	EXPECT_EQ(node->GetStandardValue(olive::MatrixGenerator::kAnchorInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));

	// Scale slider is percentage-based, floored at zero, and starts with its
	// second track disabled because uniform scale defaults to on
	EXPECT_EQ(node->GetInputProperty(olive::MatrixGenerator::kScaleInput,
									 QStringLiteral("view"))
				  .toInt(),
			  int(olive::FloatSlider::kPercentage));
	EXPECT_EQ(node->GetInputProperty(olive::MatrixGenerator::kScaleInput,
									 QStringLiteral("min"))
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));
	EXPECT_TRUE(node->GetInputProperty(olive::MatrixGenerator::kScaleInput,
									   QStringLiteral("disable1"))
					.toBool());

	// Uniform scale is a UI toggle, not a renderable parameter
	EXPECT_FALSE(node->IsInputConnectable(
		olive::MatrixGenerator::kUniformScaleInput));
	EXPECT_FALSE(node->IsInputKeyframable(
		olive::MatrixGenerator::kUniformScaleInput));
	EXPECT_TRUE(
		node->IsInputConnectable(olive::MatrixGenerator::kPositionInput));
	EXPECT_TRUE(
		node->IsInputKeyframable(olive::MatrixGenerator::kPositionInput));
}

TEST(MatrixGenerator, RetranslateSetsInputNames)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::MatrixGenerator::kPositionInput),
			  QStringLiteral("Position"));
	EXPECT_EQ(node->GetInputName(olive::MatrixGenerator::kRotationInput),
			  QStringLiteral("Rotation"));
	EXPECT_EQ(node->GetInputName(olive::MatrixGenerator::kScaleInput),
			  QStringLiteral("Scale"));
	EXPECT_EQ(node->GetInputName(olive::MatrixGenerator::kUniformScaleInput),
			  QStringLiteral("Uniform Scale"));
	EXPECT_EQ(node->GetInputName(olive::MatrixGenerator::kAnchorInput),
			  QStringLiteral("Anchor Point"));
}

TEST(MatrixGenerator, UniformScaleTogglesScaleSecondTrack)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	EXPECT_TRUE(node->GetInputProperty(olive::MatrixGenerator::kScaleInput,
									   QStringLiteral("disable1"))
					.toBool());

	node->SetStandardValue(olive::MatrixGenerator::kUniformScaleInput, false);
	EXPECT_FALSE(node->GetInputProperty(olive::MatrixGenerator::kScaleInput,
										QStringLiteral("disable1"))
					 .toBool());

	node->SetStandardValue(olive::MatrixGenerator::kUniformScaleInput, true);
	EXPECT_TRUE(node->GetInputProperty(olive::MatrixGenerator::kScaleInput,
									   QStringLiteral("disable1"))
					.toBool());
}

TEST(MatrixGenerator, DefaultValueIsIdentityMatrix)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);

	olive::NodeValueTable table = GenerateTable(node);
	olive::NodeValue value = table.Get(olive::NodeValue::kMatrix);
	ASSERT_EQ(int(value.type()), int(olive::NodeValue::kMatrix));
	EXPECT_TRUE(value.toMatrix().isIdentity());
}

TEST(MatrixGenerator, PositionTranslatesMatrix)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	node->SetStandardValue(olive::MatrixGenerator::kPositionInput,
						   QVector2D(100.0f, 50.0f));

	olive::NodeValueTable table = GenerateTable(node);
	const QVector3D mapped =
		table.Get(olive::NodeValue::kMatrix).toMatrix().map(QVector3D(0, 0, 0));
	EXPECT_FLOAT_EQ(mapped.x(), 100.0f);
	EXPECT_FLOAT_EQ(mapped.y(), 50.0f);
	EXPECT_FLOAT_EQ(mapped.z(), 0.0f);
}

TEST(MatrixGenerator, RotationAppliesAroundZAxis)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	node->SetStandardValue(olive::MatrixGenerator::kRotationInput, 90.0);

	olive::NodeValueTable table = GenerateTable(node);
	const QVector3D mapped =
		table.Get(olive::NodeValue::kMatrix).toMatrix().map(QVector3D(1, 0, 0));
	EXPECT_NEAR(mapped.x(), 0.0f, 1e-5f);
	EXPECT_NEAR(mapped.y(), 1.0f, 1e-5f);
	EXPECT_NEAR(mapped.z(), 0.0f, 1e-5f);
}

TEST(MatrixGenerator, PositionAppliesBeforeRotation)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	node->SetStandardValue(olive::MatrixGenerator::kPositionInput,
						   QVector2D(10.0f, 0.0f));
	node->SetStandardValue(olive::MatrixGenerator::kRotationInput, 90.0);

	// The transform chain is translate * rotate, so (1,0) is first rotated to
	// (0,1) and then shifted by the position
	olive::NodeValueTable table = GenerateTable(node);
	const QVector3D mapped =
		table.Get(olive::NodeValue::kMatrix).toMatrix().map(QVector3D(1, 0, 0));
	EXPECT_NEAR(mapped.x(), 10.0f, 1e-5f);
	EXPECT_NEAR(mapped.y(), 1.0f, 1e-5f);
}

TEST(MatrixGenerator, UniformScaleUsesXForBothAxes)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	node->SetStandardValue(olive::MatrixGenerator::kScaleInput,
						   QVector2D(2.0f, 3.0f));

	// Uniform scale on: the X component drives both axes
	node->SetStandardValue(olive::MatrixGenerator::kUniformScaleInput, true);
	olive::NodeValueTable table = GenerateTable(node);
	QVector3D mapped =
		table.Get(olive::NodeValue::kMatrix).toMatrix().map(QVector3D(1, 1, 0));
	EXPECT_FLOAT_EQ(mapped.x(), 2.0f);
	EXPECT_FLOAT_EQ(mapped.y(), 2.0f);

	// Uniform scale off: each axis uses its own component
	node->SetStandardValue(olive::MatrixGenerator::kUniformScaleInput, false);
	table = GenerateTable(node);
	mapped =
		table.Get(olive::NodeValue::kMatrix).toMatrix().map(QVector3D(1, 1, 0));
	EXPECT_FLOAT_EQ(mapped.x(), 2.0f);
	EXPECT_FLOAT_EQ(mapped.y(), 3.0f);
}

TEST(MatrixGenerator, AnchorPointShiftsMatrix)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::MatrixGenerator>(&project);
	node->SetStandardValue(olive::MatrixGenerator::kAnchorInput,
						   QVector2D(10.0f, 20.0f));

	olive::NodeValueTable table = GenerateTable(node);
	const QMatrix4x4 mat = table.Get(olive::NodeValue::kMatrix).toMatrix();

	// The anchor itself maps back to the origin
	const QVector3D anchor = mat.map(QVector3D(10.0f, 20.0f, 0.0f));
	EXPECT_FLOAT_EQ(anchor.x(), 0.0f);
	EXPECT_FLOAT_EQ(anchor.y(), 0.0f);

	// ...and the origin is pushed away by the anchor offset
	const QVector3D origin = mat.map(QVector3D(0, 0, 0));
	EXPECT_FLOAT_EQ(origin.x(), -10.0f);
	EXPECT_FLOAT_EQ(origin.y(), -20.0f);
}

TEST(ShapeNode, MetadataIsCorrect)
{
	olive::ShapeNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.shape"));
	EXPECT_EQ(node.Name(), QStringLiteral("Shape"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));
}

TEST(ShapeNode, InputDefaults)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::ShapeNode>(&project);

	// From GeneratorWithMerge: base texture is the effect input
	EXPECT_EQ(node->GetEffectInputID(), olive::GeneratorWithMerge::kBaseInput);
	EXPECT_TRUE(node->GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(int(node->GetInputDataType(olive::GeneratorWithMerge::kBaseInput)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		node->IsInputKeyframable(olive::GeneratorWithMerge::kBaseInput));

	// From ShapeNodeBase: position, size and color
	EXPECT_EQ(node->GetStandardValue(olive::ShapeNodeBase::kPositionInput)
				  .value<QVector2D>(),
			  QVector2D(0.0f, 0.0f));
	EXPECT_EQ(node->GetStandardValue(olive::ShapeNodeBase::kSizeInput)
				  .value<QVector2D>(),
			  QVector2D(100.0f, 100.0f));
	const olive::core::Color color =
		node->GetStandardValue(olive::ShapeNodeBase::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 0.0f);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	// Shape-specific: type combo defaults to rectangle, radius to 20
	EXPECT_EQ(int(node->GetInputDataType(olive::ShapeNode::kTypeInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node->GetStandardValue(olive::ShapeNode::kTypeInput).toInt(),
			  int(olive::ShapeNode::kRectangle));
	EXPECT_EQ(node->GetStandardValue(olive::ShapeNode::kRadiusInput).toDouble(),
			  20.0);
	EXPECT_EQ(node->GetInputProperty(olive::ShapeNode::kRadiusInput,
									 QStringLiteral("min"))
				  .toDouble(),
			  0.0);
}

TEST(ShapeNode, RetranslateSetsNamesAndComboStrings)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::ShapeNode>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::ShapeNode::kTypeInput),
			  QStringLiteral("Type"));
	EXPECT_EQ(node->GetInputName(olive::ShapeNode::kRadiusInput),
			  QStringLiteral("Radius"));
	EXPECT_EQ(node->GetInputName(olive::ShapeNodeBase::kPositionInput),
			  QStringLiteral("Position"));
	EXPECT_EQ(node->GetInputName(olive::ShapeNodeBase::kSizeInput),
			  QStringLiteral("Size"));
	EXPECT_EQ(node->GetInputName(olive::ShapeNodeBase::kColorInput),
			  QStringLiteral("Color"));
	EXPECT_EQ(node->GetInputName(olive::GeneratorWithMerge::kBaseInput),
			  QStringLiteral("Base"));

	const QStringList types =
		node->GetComboBoxStrings(olive::ShapeNode::kTypeInput);
	ASSERT_EQ(types.size(), 3);
	EXPECT_EQ(types.at(int(olive::ShapeNode::kRectangle)),
			  QStringLiteral("Rectangle"));
	EXPECT_EQ(types.at(int(olive::ShapeNode::kEllipse)),
			  QStringLiteral("Ellipse"));
	EXPECT_EQ(types.at(int(olive::ShapeNode::kRoundedRectangle)),
			  QStringLiteral("Rounded Rectangle"));
}

TEST(ShapeNode, RadiusHiddenUnlessRoundedRectangle)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::ShapeNode>(&project);

	node->SetStandardValue(olive::ShapeNode::kTypeInput,
					   int(olive::ShapeNode::kRoundedRectangle));
	EXPECT_FALSE(node->IsInputHidden(olive::ShapeNode::kRadiusInput));

	node->SetStandardValue(olive::ShapeNode::kTypeInput,
					   int(olive::ShapeNode::kEllipse));
	EXPECT_TRUE(node->IsInputHidden(olive::ShapeNode::kRadiusInput));

	node->SetStandardValue(olive::ShapeNode::kTypeInput,
					   int(olive::ShapeNode::kRectangle));
	EXPECT_TRUE(node->IsInputHidden(olive::ShapeNode::kRadiusInput));
}

TEST(ShapeNode, ShaderCodeLoadsShapeAndMergeShaders)
{
	olive::ShapeNode node;

	const olive::ShaderCode shape = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("shape")));
	EXPECT_FALSE(shape.frag_code().isEmpty());
	EXPECT_TRUE(shape.frag_code().contains(QStringLiteral("type_in")));

	// The merge shader comes from GeneratorWithMerge
	const olive::ShaderCode merge = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("mrg")));
	EXPECT_FALSE(merge.frag_code().isEmpty());
	EXPECT_TRUE(merge.frag_code().contains(QStringLiteral("blend_in")));

	// Unknown requests produce no code
	const olive::ShaderCode unknown = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("bogus")));
	EXPECT_TRUE(unknown.frag_code().isEmpty());
	EXPECT_TRUE(unknown.vert_code().isEmpty());
}

TEST(ShapeNode, ValueWithoutBasePushesShapeJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::ShapeNode>(&project);

	const olive::VideoParams vparams = TestVideoParams();
	olive::NodeValueTable table = GenerateTable(node, vparams);

	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(texture->params().height(), vparams.height());

	auto *job = dynamic_cast<olive::ShaderJob *>(texture->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->GetShaderID(), QStringLiteral("shape"));
	EXPECT_EQ(job->Get(QStringLiteral("resolution_in")).toVec2(),
			  vparams.square_resolution());
	EXPECT_EQ(job->Get(olive::ShapeNode::kTypeInput).toInt(),
			  int(olive::ShapeNode::kRectangle));
	EXPECT_EQ(job->Get(olive::ShapeNode::kRadiusInput).toDouble(), 20.0);
}

TEST(ShapeNode, ValueWithBasePushesMergeJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::ShapeNode>(&project);
	auto *constant = AddNode<ConstantTextureNode>(&project);

	const olive::TexturePtr base = std::make_shared<olive::Texture>(
		olive::VideoParams(64, 48, olive::core::PixelFormat::U8, 4));
	constant->SetTexture(base);
	olive::Node::ConnectEdge(constant,
							 olive::NodeInput(
								 node, olive::GeneratorWithMerge::kBaseInput));

	olive::NodeValueTable table = GenerateTable(node, TestVideoParams());

	// With a base connected the generator composites onto it via the "mrg"
	// merge shader
	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), base->params().width());
	EXPECT_EQ(texture->params().height(), base->params().height());

	auto *merge = dynamic_cast<olive::ShaderJob *>(texture->job());
	ASSERT_TRUE(merge);
	EXPECT_EQ(merge->GetShaderID(), QStringLiteral("mrg"));
	EXPECT_EQ(merge->Get(olive::MergeNode::kBaseIn).toTexture(), base);

	// The blend input carries the shape generation job, sized after the base
	olive::TexturePtr blend =
		merge->Get(olive::MergeNode::kBlendIn).toTexture();
	ASSERT_TRUE(blend);
	ASSERT_TRUE(blend->IsJob());
	EXPECT_EQ(blend->params().width(), base->params().width());
	auto *shape_job = dynamic_cast<olive::ShaderJob *>(blend->job());
	ASSERT_TRUE(shape_job);
	EXPECT_EQ(shape_job->GetShaderID(), QStringLiteral("shape"));
	EXPECT_EQ(shape_job->Get(QStringLiteral("resolution_in")).toVec2(),
			  base->virtual_resolution());
}

TEST(SolidGenerator, MetadataIsCorrect)
{
	olive::SolidGenerator node;
	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.solidgenerator"));
	EXPECT_EQ(node.Name(), QStringLiteral("Solid"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));
}

TEST(SolidGenerator, DefaultColorIsRed)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::SolidGenerator>(&project);
	EXPECT_EQ(int(node->GetInputDataType(olive::SolidGenerator::kColorInput)),
			  int(olive::NodeValue::kColor));

	const olive::core::Color color =
		node->GetStandardValue(olive::SolidGenerator::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 0.0f);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);
}

TEST(SolidGenerator, RetranslateSetsInputName)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::SolidGenerator>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::SolidGenerator::kColorInput),
			  QStringLiteral("Color"));
}

TEST(SolidGenerator, ShaderCodeContainsColorUniform)
{
	olive::SolidGenerator node;

	// The request is ignored, the solid shader is always returned
	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("color_in")));
}

TEST(SolidGenerator, ValuePushesShaderJobWithColor)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::SolidGenerator>(&project);
	node->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::core::Color(0.25f, 0.5f, 0.75f, 1.0f)));

	const olive::VideoParams vparams = TestVideoParams();
	olive::NodeValueTable table = GenerateTable(node, vparams);

	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(texture->params().height(), vparams.height());
	EXPECT_EQ(int(texture->params().format()),
			  int(olive::core::PixelFormat::U8));

	auto *job = dynamic_cast<olive::ShaderJob *>(texture->job());
	ASSERT_TRUE(job);
	const olive::core::Color color =
		job->Get(olive::SolidGenerator::kColorInput).toColor();
	EXPECT_FLOAT_EQ(color.red(), 0.25f);
	EXPECT_FLOAT_EQ(color.green(), 0.5f);
	EXPECT_FLOAT_EQ(color.blue(), 0.75f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);
}

TEST(NoiseGenerator, MetadataAndEffectFlags)
{
	olive::NoiseGeneratorNode node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.noise"));
	EXPECT_EQ(node.Name(), QStringLiteral("Noise"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));

	EXPECT_TRUE(node.GetFlags() & olive::Node::kVideoEffect);
	EXPECT_EQ(node.GetEffectInputID(), olive::NoiseGeneratorNode::kBaseIn);
}

TEST(NoiseGenerator, InputDefaults)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::NoiseGeneratorNode>(&project);

	EXPECT_EQ(int(node->GetInputDataType(olive::NoiseGeneratorNode::kBaseIn)),
			  int(olive::NodeValue::kTexture));
	EXPECT_FALSE(node->IsInputKeyframable(olive::NoiseGeneratorNode::kBaseIn));

	EXPECT_EQ(
		int(node->GetInputDataType(olive::NoiseGeneratorNode::kStrengthInput)),
		int(olive::NodeValue::kFloat));
	EXPECT_EQ(node->GetStandardValue(olive::NoiseGeneratorNode::kStrengthInput)
				  .toDouble(),
			  0.2);
	EXPECT_EQ(node->GetInputProperty(olive::NoiseGeneratorNode::kStrengthInput,
									 QStringLiteral("min"))
				  .toInt(),
			  0);
	EXPECT_EQ(node->GetInputProperty(olive::NoiseGeneratorNode::kStrengthInput,
									 QStringLiteral("view"))
				  .toInt(),
			  int(olive::FloatSlider::kPercentage));

	EXPECT_EQ(int(node->GetInputDataType(olive::NoiseGeneratorNode::kColorInput)),
			  int(olive::NodeValue::kBoolean));
	EXPECT_FALSE(node->GetStandardValue(olive::NoiseGeneratorNode::kColorInput)
					 .toBool());
}

TEST(NoiseGenerator, RetranslateSetsInputNames)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::NoiseGeneratorNode>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::NoiseGeneratorNode::kBaseIn),
			  QStringLiteral("Base"));
	EXPECT_EQ(node->GetInputName(olive::NoiseGeneratorNode::kStrengthInput),
			  QStringLiteral("Strength"));
	EXPECT_EQ(node->GetInputName(olive::NoiseGeneratorNode::kColorInput),
			  QStringLiteral("Color"));
}

TEST(NoiseGenerator, ShaderCodeLoads)
{
	olive::NoiseGeneratorNode node;

	const olive::ShaderCode code = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("anything")));
	EXPECT_FALSE(code.frag_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(QStringLiteral("strength_in")));
}

TEST(NoiseGenerator, ValueInsertsTimeAndUsesCacheParams)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::NoiseGeneratorNode>(&project);

	const olive::VideoParams vparams = TestVideoParams();
	const olive::TimeRange range(olive::rational(1, 2), olive::rational(3, 4));
	olive::NodeValueTable table = GenerateTable(node, vparams, range);

	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(texture->params().height(), vparams.height());

	auto *job = dynamic_cast<olive::ShaderJob *>(texture->job());
	ASSERT_TRUE(job);

	// The noise is animated by the current time
	const olive::NodeValue time = job->Get(QStringLiteral("time_in"));
	ASSERT_EQ(int(time.type()), int(olive::NodeValue::kFloat));
	EXPECT_DOUBLE_EQ(time.toDouble(), range.in().toDouble());

	EXPECT_DOUBLE_EQ(job->Get(olive::NoiseGeneratorNode::kStrengthInput)
						 .toDouble(),
					 0.2);
}

TEST(NoiseGenerator, ValueWithBaseUsesBaseParams)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::NoiseGeneratorNode>(&project);
	auto *constant = AddNode<ConstantTextureNode>(&project);

	const olive::TexturePtr base = std::make_shared<olive::Texture>(
		olive::VideoParams(64, 48, olive::core::PixelFormat::U8, 4));
	constant->SetTexture(base);
	olive::Node::ConnectEdge(
		constant, olive::NodeInput(node, olive::NoiseGeneratorNode::kBaseIn));

	olive::NodeValueTable table = GenerateTable(node, TestVideoParams());

	// The generated noise adopts the base texture's params, not the sequence's
	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	EXPECT_EQ(texture->params().width(), base->params().width());
	EXPECT_EQ(texture->params().height(), base->params().height());
}

TEST(TextGeneratorV3, MetadataIsCorrect)
{
	olive::TextGeneratorV3 node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.text3"));
	EXPECT_EQ(node.Name(), QStringLiteral("Text"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));
}

TEST(TextGeneratorV3, InputDefaultsAndFlags)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV3>(&project);

	// The default text is a formatted HTML paragraph with the placeholder
	// already substituted ("Sample Text" replaces %1 at construction)
	const QString text =
		node->GetStandardValue(olive::TextGeneratorV3::kTextInput).toString();
	EXPECT_TRUE(text.contains(QStringLiteral("Sample Text")));
	EXPECT_TRUE(text.contains(QStringLiteral("<p style=")));
	EXPECT_TRUE(node->GetInputProperty(olive::TextGeneratorV3::kTextInput,
									   QStringLiteral("vieweronly"))
					.toBool());

	// Text boxes default to 400x300 rather than ShapeNodeBase's 100x100
	EXPECT_EQ(node->GetStandardValue(olive::ShapeNodeBase::kSizeInput)
				  .value<QVector2D>(),
			  QVector2D(400.0f, 300.0f));

	// Alignment and argument inputs are hidden, non-rendered UI state
	EXPECT_TRUE(node->IsInputHidden(olive::TextGeneratorV3::kVerticalAlignmentInput));
	EXPECT_TRUE(
		node->GetInputFlags(olive::TextGeneratorV3::kVerticalAlignmentInput) &
		olive::kInputFlagStatic);
	EXPECT_EQ(node->GetStandardValue(olive::TextGeneratorV3::kVerticalAlignmentInput)
				  .toInt(),
			  int(olive::TextGeneratorV3::kVAlignTop));

	EXPECT_TRUE(node->IsInputHidden(olive::TextGeneratorV3::kUseArgsInput));
	EXPECT_TRUE(node->GetInputFlags(olive::TextGeneratorV3::kUseArgsInput) &
				olive::kInputFlagStatic);
	EXPECT_TRUE(node->GetStandardValue(olive::TextGeneratorV3::kUseArgsInput)
					.toBool());

	EXPECT_TRUE(node->InputIsArray(olive::TextGeneratorV3::kArgsInput));
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 0);
	EXPECT_EQ(node->GetInputProperty(olive::TextGeneratorV3::kArgsInput,
									 QStringLiteral("arraystart"))
				  .toInt(),
			  1);

	// TextGeneratorV3 has no color input, unlike ShapeNode
	EXPECT_FALSE(node->HasInputWithID(olive::ShapeNodeBase::kColorInput));
}

TEST(TextGeneratorV3, RetranslateSetsNamesAndComboStrings)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV3>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV3::kTextInput),
			  QStringLiteral("Text"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV3::kVerticalAlignmentInput),
			  QStringLiteral("Vertical Alignment"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV3::kArgsInput),
			  QStringLiteral("Arguments"));

	const QStringList aligns = node->GetComboBoxStrings(
		olive::TextGeneratorV3::kVerticalAlignmentInput);
	ASSERT_EQ(aligns.size(), 3);
	EXPECT_EQ(aligns.at(int(olive::TextGeneratorV3::kVAlignTop)),
			  QStringLiteral("Top"));
	EXPECT_EQ(aligns.at(int(olive::TextGeneratorV3::kVAlignMiddle)),
			  QStringLiteral("Middle"));
	EXPECT_EQ(aligns.at(int(olive::TextGeneratorV3::kVAlignBottom)),
			  QStringLiteral("Bottom"));
}

TEST(TextGeneratorV3, FormatStringSubstitutesArguments)
{
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("Hello %1"),
												   { QStringLiteral("world") }),
			  QStringLiteral("Hello world"));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(
				  QStringLiteral("%1 %2 %1"),
				  { QStringLiteral("a"), QStringLiteral("b") }),
			  QStringLiteral("a b a"));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(
				  QStringLiteral("%1%2"),
				  { QStringLiteral("a"), QStringLiteral("b") }),
			  QStringLiteral("ab"));

	// Multi-digit indices are supported
	QStringList args;
	for (int i = 0; i < 12; i++) {
		args.append(QString::number(i + 1));
	}
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%12"), args),
			  QStringLiteral("12"));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(
				  QStringLiteral("%01"), { QStringLiteral("x") }),
			  QStringLiteral("x"));
}

TEST(TextGeneratorV3, FormatStringHandlesEdgeCases)
{
	// Double percent escapes to a literal one, even without arguments
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("100%%"), {}),
			  QStringLiteral("100%"));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%%"),
												   { QStringLiteral("x") }),
			  QStringLiteral("%"));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%%%1"),
												   { QStringLiteral("x") }),
			  QStringLiteral("%x"));

	// Out-of-range and zero indices expand to nothing
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%1"), {}),
			  QStringLiteral(""));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%5"),
												   { QStringLiteral("a") }),
			  QStringLiteral(""));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%0"),
												   { QStringLiteral("a") }),
			  QStringLiteral(""));

	// A percent not followed by a digit or percent is kept literally
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("%x"),
												   { QStringLiteral("a") }),
			  QStringLiteral("%x"));
	EXPECT_EQ(olive::TextGeneratorV3::FormatString(QStringLiteral("end%"),
												   { QStringLiteral("a") }),
			  QStringLiteral("end%"));
}

TEST(TextGeneratorV3, AlignmentConversions)
{
	EXPECT_EQ(olive::TextGeneratorV3::GetQtAlignmentFromOurs(
				  olive::TextGeneratorV3::kVAlignTop),
			  Qt::AlignTop);
	EXPECT_EQ(olive::TextGeneratorV3::GetQtAlignmentFromOurs(
				  olive::TextGeneratorV3::kVAlignMiddle),
			  Qt::AlignVCenter);
	EXPECT_EQ(olive::TextGeneratorV3::GetQtAlignmentFromOurs(
				  olive::TextGeneratorV3::kVAlignBottom),
			  Qt::AlignBottom);

	// Unknown values map to no alignment
	EXPECT_EQ(olive::TextGeneratorV3::GetQtAlignmentFromOurs(
				  static_cast<olive::TextGeneratorV3::VerticalAlignment>(-1)),
			  Qt::Alignment());

	EXPECT_EQ(int(olive::TextGeneratorV3::GetOurAlignmentFromQts(Qt::AlignTop)),
			  int(olive::TextGeneratorV3::kVAlignTop));
	EXPECT_EQ(
		int(olive::TextGeneratorV3::GetOurAlignmentFromQts(Qt::AlignVCenter)),
		int(olive::TextGeneratorV3::kVAlignMiddle));
	EXPECT_EQ(
		int(olive::TextGeneratorV3::GetOurAlignmentFromQts(Qt::AlignBottom)),
		int(olive::TextGeneratorV3::kVAlignBottom));

	// Anything without a vertical component defaults to top
	EXPECT_EQ(int(olive::TextGeneratorV3::GetOurAlignmentFromQts(Qt::AlignLeft)),
			  int(olive::TextGeneratorV3::kVAlignTop));
	EXPECT_EQ(
		int(olive::TextGeneratorV3::GetOurAlignmentFromQts(Qt::AlignHCenter)),
		int(olive::TextGeneratorV3::kVAlignTop));
}

TEST(TextGeneratorV3, GetVerticalAlignmentFollowsInput)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV3>(&project);
	EXPECT_EQ(int(node->GetVerticalAlignment()),
			  int(olive::TextGeneratorV3::kVAlignTop));

	node->SetStandardValue(olive::TextGeneratorV3::kVerticalAlignmentInput,
						   int(olive::TextGeneratorV3::kVAlignBottom));
	EXPECT_EQ(int(node->GetVerticalAlignment()),
			  int(olive::TextGeneratorV3::kVAlignBottom));

	node->SetStandardValue(olive::TextGeneratorV3::kVerticalAlignmentInput,
						   int(olive::TextGeneratorV3::kVAlignMiddle));
	EXPECT_EQ(int(node->GetVerticalAlignment()),
			  int(olive::TextGeneratorV3::kVAlignMiddle));
}

TEST(TextGeneratorV3, ValueFormatsTextIntoGenerateJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV3>(&project);
	node->SetStandardValue(olive::TextGeneratorV3::kTextInput,
						   QStringLiteral("A %1 B %2"));
	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 2);
	node->SetStandardValue(olive::TextGeneratorV3::kArgsInput,
						   QStringLiteral("x"), 0);
	node->SetStandardValue(olive::TextGeneratorV3::kArgsInput,
						   QStringLiteral("y"), 1);

	// Text is always rendered to an 8-bit buffer regardless of sequence depth
	const olive::VideoParams vparams(320, 240, olive::core::PixelFormat::F32, 4);
	olive::NodeValueTable table = GenerateTable(node, vparams);

	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(int(texture->params().format()),
			  int(olive::core::PixelFormat::U8));

	auto *job = dynamic_cast<olive::GenerateJob *>(texture->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(olive::TextGeneratorV3::kTextInput).toString(),
			  QStringLiteral("A x B y"));
}

TEST(TextGeneratorV3, EmptyTextOutputsNoTextureWithoutBase)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV3>(&project);
	node->SetStandardValue(olive::TextGeneratorV3::kTextInput, QString());

	olive::NodeValueTable table = GenerateTable(node, TestVideoParams());

	// No text and no base: nothing renderable comes out
	EXPECT_TRUE(GetOutputTexture(table) == nullptr);
}

TEST(TextGeneratorV3, EmptyTextPassesBaseThrough)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV3>(&project);
	node->SetStandardValue(olive::TextGeneratorV3::kTextInput, QString());

	auto *constant = AddNode<ConstantTextureNode>(&project);
	const olive::TexturePtr base = std::make_shared<olive::Texture>(
		olive::VideoParams(64, 48, olive::core::PixelFormat::U8, 4));
	constant->SetTexture(base);
	olive::Node::ConnectEdge(constant,
							 olive::NodeInput(
								 node, olive::GeneratorWithMerge::kBaseInput));

	olive::NodeValueTable table = GenerateTable(node, TestVideoParams());

	// With empty text the base is passed through untouched instead of running
	// the text generation job
	EXPECT_EQ(GetOutputTexture(table), base);
}

TEST(PolygonGenerator, MetadataIsCorrect)
{
	olive::PolygonGenerator node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.polygon"));
	EXPECT_EQ(node.Name(), QStringLiteral("Polygon"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));
}

TEST(PolygonGenerator, DefaultPentagonPoints)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::PolygonGenerator>(&project);

	EXPECT_TRUE(node->InputIsArray(olive::PolygonGenerator::kPointsInput));
	ASSERT_EQ(node->InputArraySize(olive::PolygonGenerator::kPointsInput), 5);

	// "The Default Pentagon(tm)", as named in the implementation
	const double expected[5][2] = {
		{ 0, -135 }, { 135, -45 }, { 90, 120 }, { -90, 120 }, { -135, -45 }
	};
	for (int i = 0; i < 5; i++) {
		EXPECT_DOUBLE_EQ(node->GetSplitStandardValueOnTrack(
							 olive::PolygonGenerator::kPointsInput, 0, i)
							 .toDouble(),
						 expected[i][0])
			<< "Wrong X for point " << i;
		EXPECT_DOUBLE_EQ(node->GetSplitStandardValueOnTrack(
							 olive::PolygonGenerator::kPointsInput, 1, i)
							 .toDouble(),
						 expected[i][1])
			<< "Wrong Y for point " << i;
	}

	// Polygons default to white
	const olive::core::Color color =
		node->GetStandardValue(olive::PolygonGenerator::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 1.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);
}

TEST(PolygonGenerator, RetranslateSetsInputNames)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::PolygonGenerator>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::PolygonGenerator::kPointsInput),
			  QStringLiteral("Points"));
	EXPECT_EQ(node->GetInputName(olive::PolygonGenerator::kColorInput),
			  QStringLiteral("Color"));
	EXPECT_EQ(node->GetInputName(olive::GeneratorWithMerge::kBaseInput),
			  QStringLiteral("Base"));
}

TEST(PolygonGenerator, ShaderCodeLoadsRgbAndMergeShaders)
{
	olive::PolygonGenerator node;

	// The generated alpha mask is tinted through the rgb shader
	const olive::ShaderCode rgb = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("rgb")));
	EXPECT_FALSE(rgb.frag_code().isEmpty());
	EXPECT_TRUE(rgb.frag_code().contains(QStringLiteral("texture_in")));

	const olive::ShaderCode merge = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("mrg")));
	EXPECT_FALSE(merge.frag_code().isEmpty());
	EXPECT_TRUE(merge.frag_code().contains(QStringLiteral("blend_in")));

	const olive::ShaderCode unknown = node.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("bogus")));
	EXPECT_TRUE(unknown.frag_code().isEmpty());
	EXPECT_TRUE(unknown.vert_code().isEmpty());
}

TEST(PolygonGenerator, ValueWithoutBasePushesNestedGenerateJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::PolygonGenerator>(&project);

	const olive::VideoParams vparams(320, 240, olive::core::PixelFormat::F32, 4);
	olive::NodeValueTable table = GenerateTable(node, vparams);

	// Without a base, the output is the rgb tint job at sequence params
	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(int(texture->params().format()),
			  int(olive::core::PixelFormat::F32));

	auto *rgb = dynamic_cast<olive::ShaderJob *>(texture->job());
	ASSERT_TRUE(rgb);
	EXPECT_EQ(rgb->GetShaderID(), QStringLiteral("rgb"));

	const olive::core::Color color =
		rgb->Get(olive::PolygonGenerator::kColorInput).toColor();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 1.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	// Its texture input is the polygon's CPU-side GenerateJob, always 8-bit
	olive::TexturePtr mask = rgb->Get(QStringLiteral("texture_in")).toTexture();
	ASSERT_TRUE(mask);
	ASSERT_TRUE(mask->IsJob());
	EXPECT_EQ(int(mask->params().format()), int(olive::core::PixelFormat::U8));
	EXPECT_TRUE(dynamic_cast<olive::GenerateJob *>(mask->job()));
}
