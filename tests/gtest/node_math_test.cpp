#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "common/qtutils.h"
#include "node/globals.h"
#include "node/math/math/math.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "node/traverser.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/util/color.h"
#include "olive/core/util/rational.h"
#include "render/job/samplejob.h"

namespace
{

// Minimal node that emits a single configurable value. MathNode's inputs are
// declared as kFloat, so standard values always arrive as kFloat-typed
// NodeValues; feeding vectors, matrices, colors, rationals or sample buffers
// requires a connected node that pushes the real type.
class ConstantValueNode : public olive::Node {
public:
	ConstantValueNode() = default;

	NODE_DEFAULT_FUNCTIONS(ConstantValueNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Test Constant");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.constant");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryMath };
	}

	void SetOutput(const olive::NodeValue &value)
	{
		output_ = value;
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)
		Q_UNUSED(globals)

		table->Push(output_);
	}

private:
	olive::NodeValue output_;
};

// Traverser that resolves SampleJobs on the CPU (no audio hardware) so the
// non-static number path of MathNode can be verified end to end.
class SampleResolvingTraverser : public olive::NodeTraverser {
public:
	void Resolve(olive::NodeValue &value)
	{
		ResolveJobs(value);
	}

protected:
	virtual olive::core::SampleBuffer
	CreateSampleBuffer(const olive::core::AudioParams &params,
					   int sample_count) override
	{
		return olive::core::SampleBuffer(params, size_t(sample_count));
	}

	virtual void ProcessSamples(olive::core::SampleBuffer &destination,
								const olive::Node *node,
								const olive::TimeRange &range,
								const olive::SampleJob &job) override
	{
		Q_UNUSED(range)

		for (size_t i = 0; i < destination.sample_count(); i++) {
			node->ProcessSamples(job.GetValues(), job.samples(), destination,
								 int(i));
		}
	}
};

olive::MathNode *CreateMathNode(olive::Project *project)
{
	auto *math = new olive::MathNode();
	math->setParent(project);
	return math;
}

ConstantValueNode *CreateConstant(olive::Project *project,
								  const olive::NodeValue &value)
{
	auto *node = new ConstantValueNode();
	node->setParent(project);
	node->SetOutput(value);
	return node;
}

olive::NodeValueTable GenerateMathTable(olive::MathNode *math)
{
	olive::NodeTraverser traverser;
	return traverser.GenerateTable(
		math, olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(1, 30)));
}

olive::core::AudioParams TestAudioParams()
{
	return olive::core::AudioParams(48000, olive::core::kChannelLayoutStereo,
									olive::core::SampleFormat::F32P);
}

// Creates a stereo buffer with the given per-channel samples. Both channels
// must have the same number of samples.
olive::core::SampleBuffer MakeSampleBuffer(const std::vector<float> &channel0,
										   const std::vector<float> &channel1)
{
	olive::core::SampleBuffer buffer(TestAudioParams(), channel0.size());
	for (size_t i = 0; i < channel0.size(); i++) {
		buffer.data(0)[i] = channel0[i];
	}
	for (size_t i = 0; i < channel1.size(); i++) {
		buffer.data(1)[i] = channel1[i];
	}
	return buffer;
}

olive::NodeValue SampleValue(const olive::core::SampleBuffer &buffer)
{
	return olive::NodeValue(olive::NodeValue::kSamples,
							QVariant::fromValue(buffer));
}

} // namespace

TEST(MathNode, MetadataIsCorrect)
{
	olive::MathNode unparented;
	EXPECT_EQ(unparented.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.math"));
	EXPECT_FALSE(unparented.Description().isEmpty());
	EXPECT_TRUE(
		unparented.Category().contains(olive::Node::kCategoryMath));
	EXPECT_EQ(unparented.GetOperation(), olive::MathNode::kOpAdd);

	// Without a parent the node is just called "Math"
	EXPECT_EQ(unparented.Name(), QStringLiteral("Math"));

	// Parented nodes are named after their operation
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	EXPECT_EQ(math->Name(), QStringLiteral("Add"));

	math->SetOperation(olive::MathNode::kOpSubtract);
	EXPECT_EQ(math->GetOperation(), olive::MathNode::kOpSubtract);
	EXPECT_EQ(math->Name(), QStringLiteral("Subtract"));
}

TEST(MathNode, OperationNames)
{
	EXPECT_EQ(olive::MathNodeBase::GetOperationName(olive::MathNode::kOpAdd),
			  QStringLiteral("Add"));
	EXPECT_EQ(
		olive::MathNodeBase::GetOperationName(olive::MathNode::kOpSubtract),
		QStringLiteral("Subtract"));
	EXPECT_EQ(
		olive::MathNodeBase::GetOperationName(olive::MathNode::kOpMultiply),
		QStringLiteral("Multiply"));
	EXPECT_EQ(olive::MathNodeBase::GetOperationName(olive::MathNode::kOpDivide),
			  QStringLiteral("Divide"));
	EXPECT_EQ(olive::MathNodeBase::GetOperationName(olive::MathNode::kOpPower),
			  QStringLiteral("Power"));

	// Out-of-range operations produce an empty name
	EXPECT_TRUE(olive::MathNodeBase::GetOperationName(
					static_cast<olive::MathNode::Operation>(-1))
					.isEmpty());
}

TEST(MathNode, RetranslateSetsInputNamesAndComboStrings)
{
	olive::MathNode math;
	math.Retranslate();

	EXPECT_EQ(math.GetInputName(olive::MathNode::kMethodIn),
			  QStringLiteral("Method"));
	EXPECT_EQ(math.GetInputName(olive::MathNode::kParamAIn),
			  QStringLiteral("Value"));
	EXPECT_EQ(math.GetInputName(olive::MathNode::kParamBIn),
			  QStringLiteral("Value"));

	const QStringList operations =
		math.GetInputProperty(olive::MathNode::kMethodIn,
							  QStringLiteral("combo_str"))
			.toStringList();
	ASSERT_EQ(operations.size(), 5);
	EXPECT_EQ(operations.at(0), QStringLiteral("Add"));
	EXPECT_EQ(operations.at(1), QStringLiteral("Subtract"));
	EXPECT_EQ(operations.at(2), QStringLiteral("Multiply"));
	EXPECT_EQ(operations.at(3), QStringLiteral("Divide"));
	// The combo list matches the Operation enum exactly, so kOpPower == 4
	// selects "Power"
	EXPECT_EQ(operations.at(4), QStringLiteral("Power"));
}

TEST(MathNode, AddNumbers)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);
	math->SetStandardValue(olive::MathNode::kParamAIn, 2.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 3.0);

	olive::NodeValueTable table = GenerateMathTable(math);
	EXPECT_FLOAT_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 5.0);
}

TEST(MathNode, SubtractNumbers)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpSubtract);
	math->SetStandardValue(olive::MathNode::kParamAIn, 7.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 10.0);

	olive::NodeValueTable table = GenerateMathTable(math);
	EXPECT_FLOAT_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), -3.0);
}

TEST(MathNode, MultiplyNumbers)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);
	math->SetStandardValue(olive::MathNode::kParamAIn, 2.5);
	math->SetStandardValue(olive::MathNode::kParamBIn, 4.0);

	olive::NodeValueTable table = GenerateMathTable(math);
	EXPECT_FLOAT_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 10.0);
}

TEST(MathNode, DivideNumbers)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);
	math->SetStandardValue(olive::MathNode::kParamAIn, 7.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 2.0);

	olive::NodeValueTable table = GenerateMathTable(math);
	EXPECT_FLOAT_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 3.5);
}

TEST(MathNode, PowerNumbers)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpPower);
	math->SetStandardValue(olive::MathNode::kParamAIn, 2.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 10.0);

	olive::NodeValueTable table = GenerateMathTable(math);
	EXPECT_FLOAT_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 1024.0);
}

TEST(MathNode, DivideByZeroProducesInfinity)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);
	math->SetStandardValue(olive::MathNode::kParamAIn, 1.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 0.0);

	olive::NodeValueTable table = GenerateMathTable(math);
	const double result = table.Get(olive::NodeValue::kFloat).toDouble();
	EXPECT_TRUE(std::isinf(result));
	EXPECT_GT(result, 0.0);

	// 0 / 0 yields NaN
	math->SetStandardValue(olive::MathNode::kParamAIn, 0.0);
	table = GenerateMathTable(math);
	EXPECT_TRUE(std::isnan(table.Get(olive::NodeValue::kFloat).toDouble()));
}

TEST(MathNode, ChangingOperationChangesResult)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetStandardValue(olive::MathNode::kParamAIn, 2.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 3.0);

	const olive::MathNode::Operation ops[] = {
		olive::MathNode::kOpAdd,	olive::MathNode::kOpSubtract,
		olive::MathNode::kOpMultiply, olive::MathNode::kOpDivide,
		olive::MathNode::kOpPower
	};
	const double expected[] = { 5.0, -1.0, 6.0, 2.0 / 3.0, 8.0 };

	for (int i = 0; i < 5; i++) {
		math->SetOperation(ops[i]);
		olive::NodeValueTable table = GenerateMathTable(math);
		EXPECT_NEAR(table.Get(olive::NodeValue::kFloat).toDouble(), expected[i],
					1e-6)
			<< "Failed at operation index " << i;
	}
}

TEST(MathNode, AddSubtractRationalsPreserveRationalType)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 2))));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 4))));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	math->SetOperation(olive::MathNode::kOpAdd);
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kRational);
	ASSERT_EQ(result.type(), olive::NodeValue::kRational);
	EXPECT_EQ(result.toRational(), olive::core::rational(3, 4));

	math->SetOperation(olive::MathNode::kOpSubtract);
	table = GenerateMathTable(math);
	result = table.Get(olive::NodeValue::kRational);
	ASSERT_EQ(result.type(), olive::NodeValue::kRational);
	EXPECT_EQ(result.toRational(), olive::core::rational(1, 4));
}

TEST(MathNode, MultiplyDivideRationalsPreserveRationalType)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 2))));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 4))));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	math->SetOperation(olive::MathNode::kOpMultiply);
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kRational);
	ASSERT_EQ(result.type(), olive::NodeValue::kRational);
	EXPECT_EQ(result.toRational(), olive::core::rational(1, 8));

	math->SetOperation(olive::MathNode::kOpDivide);
	table = GenerateMathTable(math);
	result = table.Get(olive::NodeValue::kRational);
	ASSERT_EQ(result.type(), olive::NodeValue::kRational);
	EXPECT_EQ(result.toRational(), olive::core::rational(2));
}

TEST(MathNode, PowerOnRationalsProducesFloat)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpPower);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 2))));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(2))));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	// Power is not supported on rationals, so the result falls back to float
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(result.type(), olive::NodeValue::kFloat);
	EXPECT_FLOAT_EQ(result.toDouble(), 0.25);
}

TEST(MathNode, RationalDividedByZeroProducesNaN)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 2))));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(0))));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kRational);
	ASSERT_EQ(result.type(), olive::NodeValue::kRational);
	EXPECT_TRUE(result.toRational().isNaN());
}

TEST(MathNode, MixedRationalAndFloatProducesFloat)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);
	math->SetStandardValue(olive::MathNode::kParamBIn, 0.5);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kRational,
						 QVariant::fromValue(olive::core::rational(1, 2))));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	// Only rational+rational preserves the rational type
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(result.type(), olive::NodeValue::kFloat);
	EXPECT_FLOAT_EQ(result.toDouble(), 1.0);
}

TEST(MathNode, IntegerInputsProduceFloatResult)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);

	ConstantValueNode *a = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kInt, int64_t(7)));
	ConstantValueNode *b = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kInt, int64_t(6)));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	math->SetOperation(olive::MathNode::kOpMultiply);
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(result.type(), olive::NodeValue::kFloat);
	EXPECT_FLOAT_EQ(result.toDouble(), 42.0);

	// Mixed int and float
	olive::Node::DisconnectEdge(b,
								olive::NodeInput(math, olive::MathNode::kParamBIn));
	math->SetStandardValue(olive::MathNode::kParamBIn, 0.5);
	math->SetOperation(olive::MathNode::kOpAdd);
	table = GenerateMathTable(math);
	EXPECT_FLOAT_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 7.5);
}

TEST(MathNode, AddVectorsPromotesToLargerType)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec2, QVector2D(1.0f, 2.0f)));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec3,
						 QVector3D(10.0f, 20.0f, 30.0f)));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec3);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec3);
	const QVector3D vec = result.toVec3();
	EXPECT_FLOAT_EQ(vec.x(), 11.0f);
	EXPECT_FLOAT_EQ(vec.y(), 22.0f);
	EXPECT_FLOAT_EQ(vec.z(), 30.0f);
}

TEST(MathNode, SubtractVec4)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpSubtract);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec4,
						 QVector4D(5.0f, 7.0f, 9.0f, 11.0f)));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec4,
						 QVector4D(1.0f, 2.0f, 3.0f, 4.0f)));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec4);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec4);
	const QVector4D vec = result.toVec4();
	EXPECT_FLOAT_EQ(vec.x(), 4.0f);
	EXPECT_FLOAT_EQ(vec.y(), 5.0f);
	EXPECT_FLOAT_EQ(vec.z(), 6.0f);
	EXPECT_FLOAT_EQ(vec.w(), 7.0f);
}

TEST(MathNode, MultiplyDivideVectorsComponentwise)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec2, QVector2D(2.0f, 3.0f)));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec2, QVector2D(4.0f, 5.0f)));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	math->SetOperation(olive::MathNode::kOpMultiply);
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec2);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec2);
	EXPECT_FLOAT_EQ(result.toVec2().x(), 8.0f);
	EXPECT_FLOAT_EQ(result.toVec2().y(), 15.0f);

	math->SetOperation(olive::MathNode::kOpDivide);
	table = GenerateMathTable(math);
	result = table.Get(olive::NodeValue::kVec2);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec2);
	EXPECT_FLOAT_EQ(result.toVec2().x(), 0.5f);
	EXPECT_FLOAT_EQ(result.toVec2().y(), 0.6f);
}

TEST(MathNode, VectorPowerReturnsFirstInputUnchanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpPower);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec2, QVector2D(2.0f, 3.0f)));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec2, QVector2D(4.0f, 5.0f)));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	// Power is not implemented for vector/vector, the first vector is
	// returned unchanged
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec2);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec2);
	EXPECT_FLOAT_EQ(result.toVec2().x(), 2.0f);
	EXPECT_FLOAT_EQ(result.toVec2().y(), 3.0f);
}

TEST(MathNode, MultiplyVectorByNumber)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);
	math->SetStandardValue(olive::MathNode::kParamBIn, 2.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec4,
						 QVector4D(1.0f, 2.0f, 3.0f, 4.0f)));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec4);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec4);
	const QVector4D vec = result.toVec4();
	EXPECT_FLOAT_EQ(vec.x(), 2.0f);
	EXPECT_FLOAT_EQ(vec.y(), 4.0f);
	EXPECT_FLOAT_EQ(vec.z(), 6.0f);
	EXPECT_FLOAT_EQ(vec.w(), 8.0f);
}

TEST(MathNode, DivideVectorByNumber)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);
	math->SetStandardValue(olive::MathNode::kParamBIn, 2.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec3,
						 QVector3D(2.0f, 4.0f, 6.0f)));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec3);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec3);
	const QVector3D vec = result.toVec3();
	EXPECT_FLOAT_EQ(vec.x(), 1.0f);
	EXPECT_FLOAT_EQ(vec.y(), 2.0f);
	EXPECT_FLOAT_EQ(vec.z(), 3.0f);
}

TEST(MathNode, AddVectorAndNumberIsNoOp)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);
	math->SetStandardValue(olive::MathNode::kParamBIn, 5.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec2, QVector2D(1.0f, 2.0f)));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	// Only multiply/divide are implemented for vector+number; add returns
	// the vector unchanged
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec2);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec2);
	EXPECT_FLOAT_EQ(result.toVec2().x(), 1.0f);
	EXPECT_FLOAT_EQ(result.toVec2().y(), 2.0f);
}

TEST(MathNode, NumberTimesVectorScalesVector)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);
	math->SetStandardValue(olive::MathNode::kParamAIn, 2.0);

	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec4,
						 QVector4D(1.0f, 2.0f, 3.0f, 4.0f)));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	// With the number in parameter A and the vector in parameter B, the
	// number is still picked as the number operand and the vector is scaled,
	// mirroring MultiplyVectorByNumber with the operands swapped
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec4);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec4);
	const QVector4D vec = result.toVec4();
	EXPECT_FLOAT_EQ(vec.x(), 2.0f);
	EXPECT_FLOAT_EQ(vec.y(), 4.0f);
	EXPECT_FLOAT_EQ(vec.z(), 6.0f);
	EXPECT_FLOAT_EQ(vec.w(), 8.0f);
}

TEST(MathNode, MultiplyMatrixByVector)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);

	QMatrix4x4 matrix;
	matrix.scale(2.0f, 3.0f, 4.0f);

	ConstantValueNode *a = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kMatrix, matrix));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kVec4,
						 QVector4D(1.0f, 2.0f, 3.0f, 1.0f)));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kVec4);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec4);
	const QVector4D vec = result.toVec4();
	EXPECT_FLOAT_EQ(vec.x(), 2.0f);
	EXPECT_FLOAT_EQ(vec.y(), 6.0f);
	EXPECT_FLOAT_EQ(vec.z(), 12.0f);
	EXPECT_FLOAT_EQ(vec.w(), 1.0f);
}

TEST(MathNode, MatrixMatrixAddAndMultiply)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);

	QMatrix4x4 mat_a;
	mat_a.scale(2.0f, 3.0f, 4.0f);
	QMatrix4x4 mat_b;
	mat_b.scale(3.0f, 4.0f, 5.0f);

	ConstantValueNode *a = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kMatrix, mat_a));
	ConstantValueNode *b = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kMatrix, mat_b));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	math->SetOperation(olive::MathNode::kOpMultiply);
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kMatrix);
	ASSERT_EQ(result.type(), olive::NodeValue::kMatrix);
	const QMatrix4x4 product = result.toMatrix();
	EXPECT_FLOAT_EQ(product(0, 0), 6.0f);
	EXPECT_FLOAT_EQ(product(1, 1), 12.0f);
	EXPECT_FLOAT_EQ(product(2, 2), 20.0f);
	EXPECT_FLOAT_EQ(product(3, 3), 1.0f);

	math->SetOperation(olive::MathNode::kOpAdd);
	table = GenerateMathTable(math);
	result = table.Get(olive::NodeValue::kMatrix);
	ASSERT_EQ(result.type(), olive::NodeValue::kMatrix);
	const QMatrix4x4 sum = result.toMatrix();
	EXPECT_FLOAT_EQ(sum(0, 0), 5.0f);
	EXPECT_FLOAT_EQ(sum(1, 1), 7.0f);
	EXPECT_FLOAT_EQ(sum(2, 2), 9.0f);
	EXPECT_FLOAT_EQ(sum(3, 3), 2.0f);
}

TEST(MathNode, MatrixDivideReturnsFirstInputUnchanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);

	QMatrix4x4 mat_a;
	mat_a.scale(2.0f, 3.0f, 4.0f);
	QMatrix4x4 mat_b;
	mat_b.scale(9.0f, 9.0f, 9.0f);

	ConstantValueNode *a = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kMatrix, mat_a));
	ConstantValueNode *b = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kMatrix, mat_b));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	// Divide is not implemented for matrices, the first matrix is returned
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kMatrix);
	ASSERT_EQ(result.type(), olive::NodeValue::kMatrix);
	const QMatrix4x4 out = result.toMatrix();
	EXPECT_FLOAT_EQ(out(0, 0), 2.0f);
	EXPECT_FLOAT_EQ(out(1, 1), 3.0f);
	EXPECT_FLOAT_EQ(out(2, 2), 4.0f);
}

TEST(MathNode, AddAndSubtractColors)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kColor,
						 QVariant::fromValue(
							 olive::core::Color(0.1f, 0.2f, 0.3f, 0.4f))));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kColor,
						 QVariant::fromValue(
							 olive::core::Color(0.4f, 0.3f, 0.2f, 0.1f))));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	math->SetOperation(olive::MathNode::kOpAdd);
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kColor);
	ASSERT_EQ(result.type(), olive::NodeValue::kColor);
	const olive::core::Color sum = result.toColor();
	EXPECT_FLOAT_EQ(sum.red(), 0.5f);
	EXPECT_FLOAT_EQ(sum.green(), 0.5f);
	EXPECT_FLOAT_EQ(sum.blue(), 0.5f);
	EXPECT_FLOAT_EQ(sum.alpha(), 0.5f);

	math->SetOperation(olive::MathNode::kOpSubtract);
	table = GenerateMathTable(math);
	result = table.Get(olive::NodeValue::kColor);
	ASSERT_EQ(result.type(), olive::NodeValue::kColor);
	const olive::core::Color diff = result.toColor();
	EXPECT_FLOAT_EQ(diff.red(), -0.3f);
	EXPECT_FLOAT_EQ(diff.green(), -0.1f);
	EXPECT_FLOAT_EQ(diff.blue(), 0.1f);
	EXPECT_FLOAT_EQ(diff.alpha(), 0.3f);
}

TEST(MathNode, MultiplyColorsReturnsFirstInputUnchanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kColor,
						 QVariant::fromValue(
							 olive::core::Color(0.1f, 0.2f, 0.3f, 1.0f))));
	ConstantValueNode *b = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kColor,
						 QVariant::fromValue(
							 olive::core::Color(0.4f, 0.5f, 0.6f, 1.0f))));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	// Only add/subtract are implemented for colors, the first color is
	// returned unchanged
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kColor);
	ASSERT_EQ(result.type(), olive::NodeValue::kColor);
	const olive::core::Color out = result.toColor();
	EXPECT_FLOAT_EQ(out.red(), 0.1f);
	EXPECT_FLOAT_EQ(out.green(), 0.2f);
	EXPECT_FLOAT_EQ(out.blue(), 0.3f);
	EXPECT_FLOAT_EQ(out.alpha(), 1.0f);
}

TEST(MathNode, MultiplyColorByNumber)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);
	math->SetStandardValue(olive::MathNode::kParamBIn, 4.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kColor,
						 QVariant::fromValue(
							 olive::core::Color(0.25f, 0.5f, 0.75f, 1.0f))));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kColor);
	ASSERT_EQ(result.type(), olive::NodeValue::kColor);
	const olive::core::Color out = result.toColor();
	EXPECT_FLOAT_EQ(out.red(), 1.0f);
	EXPECT_FLOAT_EQ(out.green(), 2.0f);
	EXPECT_FLOAT_EQ(out.blue(), 3.0f);
	EXPECT_FLOAT_EQ(out.alpha(), 4.0f);
}

TEST(MathNode, DivideColorByNumberIsNoOp)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);
	math->SetStandardValue(olive::MathNode::kParamBIn, 4.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		olive::NodeValue(olive::NodeValue::kColor,
						 QVariant::fromValue(
							 olive::core::Color(0.25f, 0.5f, 0.75f, 1.0f))));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	// Only multiply is implemented for color+number, the color is returned
	// unchanged
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kColor);
	ASSERT_EQ(result.type(), olive::NodeValue::kColor);
	const olive::core::Color out = result.toColor();
	EXPECT_FLOAT_EQ(out.red(), 0.25f);
	EXPECT_FLOAT_EQ(out.green(), 0.5f);
	EXPECT_FLOAT_EQ(out.blue(), 0.75f);
	EXPECT_FLOAT_EQ(out.alpha(), 1.0f);
}

TEST(MathNode, NoPairingForNoneInputLeavesInputsUntouched)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);
	math->SetStandardValue(olive::MathNode::kParamAIn, 42.0);

	// A kNone value has no valid pairing, so Value() returns without pushing
	// a result; the inputs do not leak into the output table either
	ConstantValueNode *b = CreateConstant(&project, olive::NodeValue());
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	EXPECT_EQ(table.Get(olive::NodeValue::kFloat).type(),
			  olive::NodeValue::kNone);
	EXPECT_EQ(table.Get(olive::NodeValue::kVec4).type(),
			  olive::NodeValue::kNone);
}

TEST(MathNode, DisabledNodeDoesNotComputeResult)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);
	math->SetStandardValue(olive::MathNode::kParamAIn, 2.0);
	math->SetStandardValue(olive::MathNode::kParamBIn, 3.0);
	math->SetStandardValue(olive::Node::kEnabledInput, false);

	// The node is bypassed, so the result is one of the inputs rather than
	// their sum (merged input order is unspecified)
	olive::NodeValueTable table = GenerateMathTable(math);
	const double result = table.Get(olive::NodeValue::kFloat).toDouble();
	EXPECT_TRUE(result == 2.0 || result == 3.0);
}

TEST(MathNode, MultiplySamplesByStaticNumber)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);
	math->SetStandardValue(olive::MathNode::kParamBIn, 2.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 1.0f, 2.0f, 3.0f, 4.0f },
									 { 5.0f, 6.0f, 7.0f, 8.0f })));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(result.type(), olive::NodeValue::kSamples);
	const olive::core::SampleBuffer out = result.toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1) * 2.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5) * 2.0f);
	}
}

TEST(MathNode, AddZeroToSamplesIsNoOpButStillPushesBuffer)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);
	math->SetStandardValue(olive::MathNode::kParamBIn, 0.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 1.0f, 2.0f, 3.0f, 4.0f },
									 { 5.0f, 6.0f, 7.0f, 8.0f })));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	// Adding 0 is a no-op, but the (unmodified) buffer is still pushed
	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(result.type(), olive::NodeValue::kSamples);
	const olive::core::SampleBuffer out = result.toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1));
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5));
	}
}

TEST(MathNode, DivideSamplesByZeroProducesInfinity)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpDivide);
	math->SetStandardValue(olive::MathNode::kParamBIn, 0.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 1.0f, -1.0f, 0.0f, 2.0f },
									 { 1.0f, -1.0f, 0.0f, 2.0f })));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int channel = 0; channel < 2; channel++) {
		EXPECT_TRUE(std::isinf(out.data(channel)[0]));
		EXPECT_TRUE(std::isinf(out.data(channel)[1]));
		EXPECT_TRUE(std::isnan(out.data(channel)[2]));
		EXPECT_TRUE(std::isinf(out.data(channel)[3]));
	}
}

TEST(MathNode, PowerSamplesByStaticNumber)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpPower);
	math->SetStandardValue(olive::MathNode::kParamBIn, 2.0);

	ConstantValueNode *a = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 1.0f, 2.0f, 3.0f, 4.0f },
									 { 5.0f, 6.0f, 7.0f, 8.0f })));
	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], std::pow(float(i + 1), 2.0f));
		EXPECT_FLOAT_EQ(out.data(1)[i], std::pow(float(i + 5), 2.0f));
	}
}

TEST(MathNode, AddSampleBuffersMixesAndKeepsRemainder)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpAdd);

	ConstantValueNode *a = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 1.0f, 2.0f, 3.0f, 4.0f },
									 { 5.0f, 6.0f, 7.0f, 8.0f })));
	ConstantValueNode *b = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 10.0f, 20.0f }, { 50.0f, 60.0f })));

	olive::Node::ConnectEdge(a, olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(b, olive::NodeInput(math, olive::MathNode::kParamBIn));

	// The output is as long as the longer buffer: overlapping samples are
	// mixed, the remainder is copied from the longer buffer
	olive::NodeValueTable table = GenerateMathTable(math);
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	EXPECT_FLOAT_EQ(out.data(0)[0], 11.0f);
	EXPECT_FLOAT_EQ(out.data(0)[1], 22.0f);
	EXPECT_FLOAT_EQ(out.data(0)[2], 3.0f);
	EXPECT_FLOAT_EQ(out.data(0)[3], 4.0f);
	EXPECT_FLOAT_EQ(out.data(1)[0], 55.0f);
	EXPECT_FLOAT_EQ(out.data(1)[1], 66.0f);
	EXPECT_FLOAT_EQ(out.data(1)[2], 7.0f);
	EXPECT_FLOAT_EQ(out.data(1)[3], 8.0f);
}

TEST(MathNode, ConnectedNumberProducesSampleJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::MathNode *math = CreateMathNode(&project);
	math->SetOperation(olive::MathNode::kOpMultiply);

	// A connected (non-static) number produces a SampleJob instead of
	// processing the buffer immediately
	ConstantValueNode *number = CreateConstant(
		&project, olive::NodeValue(olive::NodeValue::kFloat, 3.0));
	ConstantValueNode *samples = CreateConstant(
		&project,
		SampleValue(MakeSampleBuffer({ 1.0f, 2.0f, 3.0f, 4.0f },
									 { 5.0f, 6.0f, 7.0f, 8.0f })));

	olive::Node::ConnectEdge(number,
							 olive::NodeInput(math, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(samples,
							 olive::NodeInput(math, olive::MathNode::kParamBIn));

	olive::NodeValueTable table = GenerateMathTable(math);
	olive::NodeValue result = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(result.type(), olive::NodeValue::kSamples);
	ASSERT_TRUE(result.canConvert<olive::SampleJob>());

	// Resolve the job on the CPU and verify the processed samples
	SampleResolvingTraverser resolver;
	resolver.Resolve(result);

	const olive::core::SampleBuffer out = result.toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1) * 3.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5) * 3.0f);
	}
}

TEST(MathNode, ProcessSamplesAppliesOperationPerSample)
{
	olive::MathNode math;
	math.SetOperation(olive::MathNode::kOpMultiply);

	olive::NodeValueRow row;
	row.insert(olive::MathNode::kParamAIn,
			   olive::NodeValue(olive::NodeValue::kFloat, 3.0));

	olive::core::SampleBuffer input(TestAudioParams(), 2);
	olive::core::SampleBuffer output(TestAudioParams(), 2);
	input.data(0)[0] = 1.5f;
	input.data(0)[1] = -2.0f;
	input.data(1)[0] = 0.25f;
	input.data(1)[1] = 8.0f;

	math.ProcessSamples(row, input, output, 0);
	math.ProcessSamples(row, input, output, 1);

	EXPECT_FLOAT_EQ(output.data(0)[0], 4.5f);
	EXPECT_FLOAT_EQ(output.data(0)[1], -6.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], 0.75f);
	EXPECT_FLOAT_EQ(output.data(1)[1], 24.0f);
}

TEST(MathNode, ProcessSamplesUsesSecondParamWhenFirstMissing)
{
	olive::MathNode math;
	math.SetOperation(olive::MathNode::kOpSubtract);

	olive::NodeValueRow row;
	row.insert(olive::MathNode::kParamBIn,
			   olive::NodeValue(olive::NodeValue::kFloat, 4.0));

	olive::core::SampleBuffer input(TestAudioParams(), 1);
	olive::core::SampleBuffer output(TestAudioParams(), 1);
	input.data(0)[0] = 10.0f;
	input.data(1)[0] = 1.0f;

	math.ProcessSamples(row, input, output, 0);

	EXPECT_FLOAT_EQ(output.data(0)[0], 6.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], -3.0f);
}

TEST(MathNode, ProcessSamplesWithoutNumberLeavesOutputUntouched)
{
	olive::MathNode math;
	math.SetOperation(olive::MathNode::kOpMultiply);

	// Neither parameter carries a number: output must not be written
	olive::NodeValueRow row;

	olive::core::SampleBuffer input(TestAudioParams(), 1);
	olive::core::SampleBuffer output(TestAudioParams(), 1);
	input.data(0)[0] = 10.0f;
	output.data(0)[0] = 123.0f;
	output.data(1)[0] = 45.0f;

	math.ProcessSamples(row, input, output, 0);

	EXPECT_FLOAT_EQ(output.data(0)[0], 123.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], 45.0f);
}

TEST(MathNode, ShaderCodeForNumberAdd)
{
	olive::MathNode math;

	const olive::ShaderCode code =
		math.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("0.0.2.2")));

	EXPECT_TRUE(code.vert_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("uniform float param_a_in;")));
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("uniform float param_b_in;")));
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("vec4 c = param_a_in + param_b_in;")));
}

TEST(MathNode, ShaderCodeForTextureNumberPower)
{
	olive::MathNode math;

	// kOpPower / kPairTextureNumber / kTexture / kFloat
	const olive::ShaderCode code =
		math.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("4.8.10.2")));

	EXPECT_TRUE(code.vert_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("uniform sampler2D param_a_in;")));
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("uniform float param_b_in;")));
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("pow(texture(param_a_in, ove_texcoord), vec4(param_b_in))")));
}

TEST(MathNode, ShaderCodeForColorPairUsesVec4Uniforms)
{
	olive::MathNode math;

	// kOpAdd / kPairColorColor / kColor / kColor
	const olive::ShaderCode code =
		math.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("0.7.5.5")));

	EXPECT_TRUE(code.vert_code().isEmpty());
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("uniform vec4 param_a_in;")));
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("uniform vec4 param_b_in;")));
	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("vec4 c = param_a_in + param_b_in;")));
}

TEST(MathNode, ShaderCodeForTextureMatrixMultiplyHasVertexShader)
{
	olive::MathNode math;

	// kOpMultiply / kPairTextureMatrix / kTexture / kMatrix
	olive::ShaderCode code =
		math.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("2.10.10.6")));

	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("texture(param_a_in, ove_texcoord)")));
	EXPECT_TRUE(code.vert_code().contains(
		QStringLiteral("uniform mat4 param_b_in;")));
	EXPECT_TRUE(code.vert_code().contains(
		QStringLiteral("gl_Position = param_b_in * a_position;")));

	// Reversed operand order swaps the roles of the parameters
	code = math.GetShaderCode(
		olive::Node::ShaderRequest(QStringLiteral("2.10.6.10")));

	EXPECT_TRUE(code.frag_code().contains(
		QStringLiteral("texture(param_b_in, ove_texcoord)")));
	EXPECT_TRUE(code.vert_code().contains(
		QStringLiteral("uniform mat4 param_a_in;")));
	EXPECT_TRUE(code.vert_code().contains(
		QStringLiteral("gl_Position = param_a_in * a_position;")));
}
