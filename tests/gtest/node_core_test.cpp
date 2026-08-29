#include <gtest/gtest.h>

#include <climits>
#include <memory>

#include <QPointF>
#include <QString>
#include <QVector>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv3.h"
#include "node/keyframe.h"
#include "node/math/math/math.h"
#include "node/node.h"
#include "node/project.h"
#include "node/project/folder/folder.h"

namespace
{

// Node that records every InvalidateCache() call so the invalidation flow
// through the graph can be asserted without any rendering.
class RecordingNode : public olive::Node {
public:
	RecordingNode()
	{
		AddInput(kTestInput, olive::NodeValue::kFloat, 0.0);
	}

	NODE_DEFAULT_FUNCTIONS(RecordingNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Recording");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.recording");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryMath };
	}

	virtual void InvalidateCache(
		const olive::TimeRange &range, const QString &from, int element,
		olive::Node::InvalidateCacheOptions options) override
	{
		invalidations.append({ range, from, element });
		olive::Node::InvalidateCache(range, from, element, options);
	}

	struct Invalidation {
		olive::TimeRange range;
		QString from;
		int element;
	};

	QVector<Invalidation> invalidations;

	static const QString kTestInput;
};

const QString RecordingNode::kTestInput = QStringLiteral("test_in");

} // namespace

class NodeCoreTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	template <typename T> T *AddNode()
	{
		T *node = new T();
		node->setParent(project_.get());
		return node;
	}

	std::unique_ptr<olive::Project> project_;
};

TEST_F(NodeCoreTest, InputArrayResizeEmitsSignals)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	const int base = node->InputArraySize(olive::TextGeneratorV3::kArgsInput);

	struct ResizeEvent {
		QString input;
		int old_size;
		int new_size;
	};
	QVector<ResizeEvent> resizes;
	QObject::connect(node, &olive::Node::InputArraySizeChanged,
					 [&resizes](const QString &input, int old_size,
								int new_size) {
						 resizes.append({ input, old_size, new_size });
					 });
	int value_changed = 0;
	QObject::connect(node, &olive::Node::ValueChanged,
					 [&value_changed](const olive::NodeInput &,
									  const olive::TimeRange &) {
						 ++value_changed;
					 });

	node->InputArrayAppend(olive::TextGeneratorV3::kArgsInput);
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 1);
	ASSERT_EQ(resizes.size(), 1);
	EXPECT_EQ(resizes.first().input, olive::TextGeneratorV3::kArgsInput);
	EXPECT_EQ(resizes.first().old_size, base);
	EXPECT_EQ(resizes.first().new_size, base + 1);
	EXPECT_EQ(value_changed, 1);

	// Resizing to the current size is a no-op
	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, base + 1);
	EXPECT_EQ(resizes.size(), 1);
	EXPECT_EQ(value_changed, 1);

	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, base + 3);
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 3);
	ASSERT_EQ(resizes.size(), 2);
	EXPECT_EQ(resizes.at(1).old_size, base + 1);
	EXPECT_EQ(resizes.at(1).new_size, base + 3);

	node->InputArrayRemoveLast(olive::TextGeneratorV3::kArgsInput);
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 2);

	node->InputArrayPrepend(olive::TextGeneratorV3::kArgsInput);
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 3);
}

TEST_F(NodeCoreTest, InputArrayInsertShiftsConnectionsAndValues)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	auto *output = AddNode<olive::MathNode>();

	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 2);
	node->SetStandardValue(
		olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 0),
		QStringLiteral("zero"));
	node->SetStandardValue(
		olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 1),
		QStringLiteral("one"));
	olive::Node::ConnectEdge(
		output, olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 0));

	node->InputArrayInsert(olive::TextGeneratorV3::kArgsInput, 0);

	ASSERT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 3);

	// The connection moved down along with its element
	EXPECT_EQ(node->GetConnectedOutput(olive::TextGeneratorV3::kArgsInput, 0),
			  nullptr);
	EXPECT_EQ(node->GetConnectedOutput(olive::TextGeneratorV3::kArgsInput, 1),
			  output);
	ASSERT_EQ(output->output_connections().size(), 1);
	EXPECT_EQ(output->output_connections().front().second,
			  olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 1));

	// Values moved down too; the inserted element holds the default value
	EXPECT_TRUE(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput,
											0)
					.at(0)
					.toString()
					.isEmpty());
	EXPECT_EQ(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput,
										  1)
				  .at(0)
				  .toString(),
			  QStringLiteral("zero"));
	EXPECT_EQ(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput,
										  2)
				  .at(0)
				  .toString(),
			  QStringLiteral("one"));
}

TEST_F(NodeCoreTest, InputArrayRemoveShiftsConnectionsAndValues)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	auto *output = AddNode<olive::MathNode>();

	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 3);
	node->SetStandardValue(
		olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 0),
		QStringLiteral("zero"));
	node->SetStandardValue(
		olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 1),
		QStringLiteral("one"));
	node->SetStandardValue(
		olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 2),
		QStringLiteral("two"));
	olive::Node::ConnectEdge(
		output, olive::NodeInput(node, olive::TextGeneratorV3::kArgsInput, 1));

	node->InputArrayRemove(olive::TextGeneratorV3::kArgsInput, 0);

	ASSERT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 2);

	// The connection moved up along with its element
	EXPECT_EQ(node->GetConnectedOutput(olive::TextGeneratorV3::kArgsInput, 0),
			  output);
	EXPECT_EQ(node->GetConnectedOutput(olive::TextGeneratorV3::kArgsInput, 1),
			  nullptr);

	// Values moved up too
	EXPECT_EQ(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput,
										  0)
				  .at(0)
				  .toString(),
			  QStringLiteral("one"));
	EXPECT_EQ(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput,
										  1)
				  .at(0)
				  .toString(),
			  QStringLiteral("two"));

	// Removing the element an edge points at drops the edge entirely
	node->InputArrayRemove(olive::TextGeneratorV3::kArgsInput, 0);
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 1);
	EXPECT_EQ(node->GetConnectedOutput(olive::TextGeneratorV3::kArgsInput, 0),
			  nullptr);
	EXPECT_TRUE(output->output_connections().empty());
	EXPECT_EQ(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput,
										  0)
				  .at(0)
				  .toString(),
			  QStringLiteral("two"));
}

TEST_F(NodeCoreTest, InputFlagsReflectDeclaration)
{
	auto *math = AddNode<olive::MathNode>();
	EXPECT_TRUE(math->IsInputConnectable(olive::MathNode::kParamAIn));
	EXPECT_TRUE(math->IsInputKeyframable(olive::MathNode::kParamAIn));
	EXPECT_FALSE(math->IsInputHidden(olive::MathNode::kParamAIn));
	EXPECT_FALSE(math->InputIsArray(olive::MathNode::kParamAIn));

	// kMethodIn is declared not-connectable and not-keyframable
	EXPECT_FALSE(math->IsInputConnectable(olive::MathNode::kMethodIn));
	EXPECT_FALSE(math->IsInputKeyframable(olive::MathNode::kMethodIn));

	auto *text = AddNode<olive::TextGeneratorV3>();
	EXPECT_TRUE(
		text->IsInputHidden(olive::TextGeneratorV3::kVerticalAlignmentInput));
	EXPECT_FALSE(text->IsInputConnectable(
		olive::TextGeneratorV3::kVerticalAlignmentInput));
	EXPECT_TRUE(text->InputIsArray(olive::TextGeneratorV3::kArgsInput));
	EXPECT_FALSE(text->InputIsArray(olive::TextGeneratorV3::kTextInput));
}

TEST_F(NodeCoreTest, SetInputFlagTogglesAndEmits)
{
	auto *node = AddNode<olive::MathNode>();

	int emissions = 0;
	QString last_input;
	uint64_t last_flags = 0;
	QObject::connect(node, &olive::Node::InputFlagsChanged,
					 [&emissions, &last_input,
					  &last_flags](const QString &input,
								   const olive::InputFlags &flags) {
						 ++emissions;
						 last_input = input;
						 last_flags = flags.value();
					 });

	node->SetInputFlag(olive::MathNode::kParamAIn, olive::kInputFlagHidden);
	EXPECT_TRUE(node->IsInputHidden(olive::MathNode::kParamAIn));
	EXPECT_EQ(emissions, 1);
	EXPECT_EQ(last_input, olive::MathNode::kParamAIn);
	EXPECT_TRUE(last_flags & olive::kInputFlagHidden);

	// Setting another flag preserves the flags already set
	node->SetInputFlag(olive::MathNode::kParamAIn,
					   olive::kInputFlagNotConnectable);
	EXPECT_FALSE(node->IsInputConnectable(olive::MathNode::kParamAIn));
	EXPECT_TRUE(node->IsInputHidden(olive::MathNode::kParamAIn));
	EXPECT_EQ(emissions, 2);

	node->SetInputFlag(olive::MathNode::kParamAIn, olive::kInputFlagHidden,
					   false);
	EXPECT_FALSE(node->IsInputHidden(olive::MathNode::kParamAIn));
	EXPECT_EQ(emissions, 3);
}

TEST_F(NodeCoreTest, SetKeyframingOnNonKeyframableInputIsIgnored)
{
	auto *node = AddNode<olive::MathNode>();
	ASSERT_FALSE(node->IsInputKeyframable(olive::MathNode::kMethodIn));

	node->SetInputIsKeyframing(olive::MathNode::kMethodIn, true);
	EXPECT_FALSE(node->IsInputKeyframing(olive::MathNode::kMethodIn));
}

TEST_F(NodeCoreTest, UnknownInputAccessorsFallBackSafely)
{
	olive::MathNode node;
	const QString bogus = QStringLiteral("bogus_in");

	// All of these go through GetInternalInputData(), which must handle the
	// input not existing without crashing
	EXPECT_EQ(node.GetInputFlags(bogus).value(),
			  static_cast<uint64_t>(olive::kInputFlagNormal));
	EXPECT_EQ(node.GetInputDataType(bogus), olive::NodeValue::kNone);
	EXPECT_TRUE(node.GetInputName(bogus).isEmpty());
	EXPECT_EQ(node.GetImmediate(bogus, -1), nullptr);
	EXPECT_EQ(node.GetConnectedOutput(bogus), nullptr);
	EXPECT_FALSE(node.IsInputKeyframing(bogus));
	EXPECT_FALSE(node.HasInputProperty(bogus, QStringLiteral("x")));
	EXPECT_TRUE(node.GetInputProperties(bogus).isEmpty());
	EXPECT_TRUE(node.GetSplitStandardValue(bogus).isEmpty());
	EXPECT_TRUE(node.GetSplitDefaultValue(bogus).isEmpty());
	EXPECT_EQ(node.InputArraySize(bogus), 0);
}

TEST_F(NodeCoreTest, InputNameTypePropertiesAndDefaults)
{
	auto *node = AddNode<olive::MathNode>();

	// Default values round-trip through the split representation
	EXPECT_DOUBLE_EQ(
		node->GetDefaultValue(olive::MathNode::kParamAIn).toDouble(), 0.0);
	node->SetDefaultValue(olive::MathNode::kParamAIn, 1.5);
	EXPECT_DOUBLE_EQ(
		node->GetDefaultValue(olive::MathNode::kParamAIn).toDouble(), 1.5);
	EXPECT_DOUBLE_EQ(node->GetSplitDefaultValue(olive::MathNode::kParamAIn)
						 .at(0)
						 .toDouble(),
					 1.5);

	// Declared data type, keyframe track count and input properties
	EXPECT_EQ(node->GetInputDataType(olive::MathNode::kParamAIn),
			  olive::NodeValue::kFloat);
	EXPECT_EQ(node->GetNumberOfKeyframeTracks(olive::MathNode::kParamAIn), 1);
	EXPECT_EQ(node->GetInputProperty(olive::MathNode::kParamAIn,
									 QStringLiteral("decimalplaces"))
				  .toInt(),
			  8);

	auto *solid = AddNode<olive::SolidGenerator>();
	EXPECT_EQ(
		solid->GetNumberOfKeyframeTracks(olive::SolidGenerator::kColorInput),
		4);

	int name_emissions = 0;
	int type_emissions = 0;
	int property_emissions = 0;
	QObject::connect(node, &olive::Node::InputNameChanged,
					 [&name_emissions](const QString &, const QString &) {
						 ++name_emissions;
					 });
	QObject::connect(node, &olive::Node::InputDataTypeChanged,
					 [&type_emissions](const QString &, olive::NodeValue::Type) {
						 ++type_emissions;
					 });
	QObject::connect(node, &olive::Node::InputPropertyChanged,
					 [&property_emissions](const QString &, const QString &,
										   const QVariant &) {
						 ++property_emissions;
					 });

	node->SetInputName(olive::MathNode::kParamAIn, QStringLiteral("Custom"));
	EXPECT_EQ(node->GetInputName(olive::MathNode::kParamAIn),
			  QStringLiteral("Custom"));
	EXPECT_EQ(name_emissions, 1);

	node->SetInputDataType(olive::MathNode::kParamAIn, olive::NodeValue::kInt);
	EXPECT_EQ(node->GetInputDataType(olive::MathNode::kParamAIn),
			  olive::NodeValue::kInt);
	EXPECT_EQ(type_emissions, 1);

	node->SetInputProperty(olive::MathNode::kParamAIn, QStringLiteral("mykey"),
						   42);
	EXPECT_TRUE(node->HasInputProperty(olive::MathNode::kParamAIn,
									   QStringLiteral("mykey")));
	EXPECT_EQ(node->GetInputProperty(olive::MathNode::kParamAIn,
									 QStringLiteral("mykey"))
				  .toInt(),
			  42);
	EXPECT_TRUE(node->GetInputProperties(olive::MathNode::kParamAIn)
					.contains(QStringLiteral("decimalplaces")));
	EXPECT_EQ(property_emissions, 1);
}

TEST_F(NodeCoreTest, ContextPositionLifecycleEmitsSignals)
{
	auto *node = AddNode<olive::MathNode>();
	auto *context = AddNode<olive::Folder>();

	int added_count = 0;
	int removed_count = 0;
	QVector<QPointF> positions;
	QObject::connect(context, &olive::Node::NodeAddedToContext,
					 [&added_count](olive::Node *) { ++added_count; });
	QObject::connect(context, &olive::Node::NodeRemovedFromContext,
					 [&removed_count](olive::Node *) { ++removed_count; });
	QObject::connect(context, &olive::Node::NodePositionInContextChanged,
					 [&positions](olive::Node *, const QPointF &pos) {
						 positions.append(pos);
					 });

	EXPECT_FALSE(context->ContextContainsNode(node));
	EXPECT_TRUE(context->GetContextPositions().isEmpty());

	// First insertion reports that the node was added
	EXPECT_TRUE(context->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(3.0, 4.0), true)));
	EXPECT_TRUE(context->ContextContainsNode(node));
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(3.0, 4.0));
	EXPECT_TRUE(context->IsNodeExpandedInContext(node));
	EXPECT_EQ(added_count, 1);
	ASSERT_EQ(positions.size(), 1);
	EXPECT_EQ(positions.first(), QPointF(3.0, 4.0));

	// Updating an existing entry reports no addition and keeps the expanded
	// state
	EXPECT_FALSE(context->SetNodePositionInContext(node, QPointF(5.0, 6.0)));
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(5.0, 6.0));
	EXPECT_TRUE(context->IsNodeExpandedInContext(node));
	EXPECT_EQ(added_count, 1);
	ASSERT_EQ(positions.size(), 2);
	EXPECT_EQ(positions.at(1), QPointF(5.0, 6.0));

	context->SetNodeExpandedInContext(node, false);
	EXPECT_FALSE(context->IsNodeExpandedInContext(node));

	EXPECT_TRUE(context->RemoveNodeFromContext(node));
	EXPECT_FALSE(context->ContextContainsNode(node));
	EXPECT_EQ(removed_count, 1);

	// Removing a node that is not in the context is a no-op
	EXPECT_FALSE(context->RemoveNodeFromContext(node));
	EXPECT_EQ(removed_count, 1);
}

TEST_F(NodeCoreTest, ContextPositionSaveLoadAndOperators)
{
	const olive::Node::Position pos(QPointF(1.5, -2.5), true);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("position"));
	pos.save(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("position"));

	olive::Node::Position loaded;
	ASSERT_TRUE(loaded.load(&reader));
	EXPECT_EQ(loaded.position, QPointF(1.5, -2.5));
	EXPECT_TRUE(loaded.expanded);

	// A position missing a coordinate fails to load
	QXmlStreamReader bad_reader(
		QStringLiteral("<position><x>1</x></position>"));
	ASSERT_TRUE(bad_reader.readNextStartElement());
	olive::Node::Position incomplete;
	EXPECT_FALSE(incomplete.load(&bad_reader));

	// Position arithmetic only affects the point, not the expanded state
	const olive::Node::Position a(QPointF(1.0, 2.0));
	const olive::Node::Position b(QPointF(3.0, 4.0));
	EXPECT_EQ((a + b).position, QPointF(4.0, 6.0));
	EXPECT_EQ((b - a).position, QPointF(2.0, 2.0));

	olive::Node::Position c(QPointF(1.0, 1.0));
	c += a;
	EXPECT_EQ(c.position, QPointF(2.0, 3.0));
	c -= b;
	EXPECT_EQ(c.position, QPointF(-1.0, -1.0));
}

TEST_F(NodeCoreTest, LinkAndUnlinkLifecycle)
{
	auto *a = AddNode<olive::MathNode>();
	auto *b = AddNode<olive::SolidGenerator>();

	int a_changes = 0;
	int b_changes = 0;
	QObject::connect(a, &olive::Node::LinksChanged,
					 [&a_changes]() { ++a_changes; });
	QObject::connect(b, &olive::Node::LinksChanged,
					 [&b_changes]() { ++b_changes; });

	EXPECT_FALSE(olive::Node::AreLinked(a, b));
	EXPECT_FALSE(a->HasLinks());
	EXPECT_FALSE(b->HasLinks());

	// Invalid pairs are rejected
	EXPECT_FALSE(olive::Node::Link(a, a));
	EXPECT_FALSE(olive::Node::Link(a, nullptr));
	EXPECT_FALSE(olive::Node::Link(nullptr, b));

	EXPECT_TRUE(olive::Node::Link(a, b));
	EXPECT_TRUE(olive::Node::AreLinked(a, b));
	EXPECT_TRUE(olive::Node::AreLinked(b, a));
	EXPECT_TRUE(a->HasLinks());
	ASSERT_EQ(a->links().size(), 1);
	EXPECT_EQ(a->links().first(), b);
	EXPECT_EQ(b->links().first(), a);
	EXPECT_EQ(a_changes, 1);
	EXPECT_EQ(b_changes, 1);

	// Linking an already-linked pair does nothing
	EXPECT_FALSE(olive::Node::Link(a, b));
	EXPECT_EQ(a_changes, 1);
	EXPECT_EQ(b_changes, 1);

	EXPECT_TRUE(olive::Node::Unlink(a, b));
	EXPECT_FALSE(olive::Node::AreLinked(a, b));
	EXPECT_FALSE(a->HasLinks());
	EXPECT_EQ(a_changes, 2);
	EXPECT_EQ(b_changes, 2);

	// Unlinking an unlinked pair does nothing
	EXPECT_FALSE(olive::Node::Unlink(a, b));
	EXPECT_EQ(a_changes, 2);
	EXPECT_EQ(b_changes, 2);
}

TEST_F(NodeCoreTest, OverrideColorEmitsOnlyOnChange)
{
	auto *node = AddNode<olive::MathNode>();
	ASSERT_EQ(node->GetOverrideColor(), -1);

	int emissions = 0;
	QObject::connect(node, &olive::Node::ColorChanged,
					 [&emissions]() { ++emissions; });

	node->SetOverrideColor(4);
	EXPECT_EQ(node->GetOverrideColor(), 4);
	EXPECT_EQ(emissions, 1);

	// Setting the same color again is not a change
	node->SetOverrideColor(4);
	EXPECT_EQ(emissions, 1);

	node->SetOverrideColor(-1);
	EXPECT_EQ(node->GetOverrideColor(), -1);
	EXPECT_EQ(emissions, 2);
}

TEST_F(NodeCoreTest, ValueHintAccessorsEmitSignal)
{
	auto *node = AddNode<olive::MathNode>();

	// The default hint is empty
	const olive::Node::ValueHint def =
		node->GetValueHintForInput(olive::MathNode::kParamAIn);
	EXPECT_TRUE(def.types().isEmpty());
	EXPECT_EQ(def.index(), -1);
	EXPECT_TRUE(def.tag().isEmpty());

	QVector<olive::NodeInput> hinted;
	QObject::connect(node, &olive::Node::InputValueHintChanged,
					 [&hinted](const olive::NodeInput &input) {
						 hinted.append(input);
					 });

	const olive::Node::ValueHint hint({ olive::NodeValue::kVec2 }, 3,
									  QStringLiteral("tag"));
	node->SetValueHintForInput(olive::MathNode::kParamAIn, hint);

	const olive::Node::ValueHint stored =
		node->GetValueHintForInput(olive::MathNode::kParamAIn);
	ASSERT_EQ(stored.types().size(), 1);
	EXPECT_EQ(stored.types().first(), olive::NodeValue::kVec2);
	EXPECT_EQ(stored.index(), 3);
	EXPECT_EQ(stored.tag(), QStringLiteral("tag"));

	ASSERT_EQ(hinted.size(), 1);
	EXPECT_EQ(hinted.first(),
			  olive::NodeInput(node, olive::MathNode::kParamAIn, -1));
	EXPECT_FALSE(node->GetValueHints().isEmpty());

	// Hints are tracked per element
	node->SetValueHintForInput(olive::MathNode::kParamAIn,
							   olive::Node::ValueHint(QStringLiteral("elem")),
							   2);
	EXPECT_EQ(node->GetValueHintForInput(olive::MathNode::kParamAIn, 2).tag(),
			  QStringLiteral("elem"));
	EXPECT_EQ(node->GetValueHintForInput(olive::MathNode::kParamAIn, 1).index(),
			  -1);
}

TEST_F(NodeCoreTest, ValueHintSaveLoadRoundTrip)
{
	const olive::Node::ValueHint hint(
		{ olive::NodeValue::kVec2, olive::NodeValue::kTexture }, 7,
		QStringLiteral("mytag"));

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("hint"));
	hint.save(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("hint"));

	olive::Node::ValueHint loaded;
	ASSERT_TRUE(loaded.load(&reader));
	ASSERT_EQ(loaded.types().size(), 2);
	EXPECT_EQ(loaded.types().at(0), olive::NodeValue::kVec2);
	EXPECT_EQ(loaded.types().at(1), olive::NodeValue::kTexture);
	EXPECT_EQ(loaded.index(), 7);
	EXPECT_EQ(loaded.tag(), QStringLiteral("mytag"));

	// A default-constructed hint round-trips to an empty hint
	QString empty_xml;
	QXmlStreamWriter empty_writer(&empty_xml);
	empty_writer.writeStartDocument();
	empty_writer.writeStartElement(QStringLiteral("hint"));
	olive::Node::ValueHint().save(&empty_writer);
	empty_writer.writeEndElement();
	empty_writer.writeEndDocument();

	QXmlStreamReader empty_reader(empty_xml);
	ASSERT_TRUE(empty_reader.readNextStartElement());
	olive::Node::ValueHint empty_loaded;
	ASSERT_TRUE(empty_loaded.load(&empty_reader));
	EXPECT_TRUE(empty_loaded.types().isEmpty());
	EXPECT_EQ(empty_loaded.index(), -1);
	EXPECT_TRUE(empty_loaded.tag().isEmpty());
}

TEST_F(NodeCoreTest, EdgeAndValueChangesPropagateInvalidateCache)
{
	auto *src = AddNode<olive::MathNode>();
	auto *dst = AddNode<RecordingNode>();
	const olive::NodeInput dst_input(dst, RecordingNode::kTestInput);

	const olive::rational kMin(INT_MIN);
	const olive::rational kMax(INT_MAX);

	QVector<olive::NodeInput> value_changed_inputs;
	QObject::connect(src, &olive::Node::ValueChanged,
					 [&value_changed_inputs](const olive::NodeInput &input,
											 const olive::TimeRange &) {
						 value_changed_inputs.append(input);
					 });

	// Connecting an edge invalidates the destination over the full range
	olive::Node::ConnectEdge(src, dst_input);
	ASSERT_EQ(dst->invalidations.size(), 1);
	EXPECT_EQ(dst->invalidations.first().from, RecordingNode::kTestInput);
	EXPECT_EQ(dst->invalidations.first().element, -1);
	EXPECT_EQ(dst->invalidations.first().range.in(), kMin);
	EXPECT_EQ(dst->invalidations.first().range.out(), kMax);

	// Changing a value upstream emits ValueChanged and invalidates downstream
	src->SetStandardValue(olive::MathNode::kParamAIn, 2.0);
	ASSERT_EQ(value_changed_inputs.size(), 1);
	EXPECT_EQ(value_changed_inputs.first(),
			  olive::NodeInput(src, olive::MathNode::kParamAIn));
	ASSERT_EQ(dst->invalidations.size(), 2);
	EXPECT_EQ(dst->invalidations.at(1).range.in(), kMin);
	EXPECT_EQ(dst->invalidations.at(1).range.out(), kMax);

	// Disconnecting the edge invalidates the destination again
	olive::Node::DisconnectEdge(src, dst_input);
	ASSERT_EQ(dst->invalidations.size(), 3);
	EXPECT_TRUE(dst->input_connections().empty());
	EXPECT_TRUE(src->output_connections().empty());
}

TEST_F(NodeCoreTest, IgnoreInvalidationsFlagSuppressesInvalidation)
{
	auto *src = AddNode<olive::MathNode>();
	auto *dst = AddNode<RecordingNode>();
	const olive::NodeInput dst_input(dst, RecordingNode::kTestInput);

	// The flag on the destination input suppresses connect/disconnect
	// invalidation
	dst->SetInputFlag(RecordingNode::kTestInput,
					  olive::kInputFlagIgnoreInvalidations);
	olive::Node::ConnectEdge(src, dst_input);
	EXPECT_TRUE(dst->invalidations.isEmpty());

	// The flag on the source input suppresses value-change propagation, but
	// the ValueChanged signal is still emitted
	int value_changed_count = 0;
	QObject::connect(src, &olive::Node::ValueChanged,
					 [&value_changed_count](const olive::NodeInput &,
											const olive::TimeRange &) {
						 ++value_changed_count;
					 });
	src->SetInputFlag(olive::MathNode::kParamAIn,
					  olive::kInputFlagIgnoreInvalidations);
	src->SetStandardValue(olive::MathNode::kParamAIn, 5.0);
	EXPECT_EQ(value_changed_count, 1);
	EXPECT_TRUE(dst->invalidations.isEmpty());

	olive::Node::DisconnectEdge(src, dst_input);
	EXPECT_TRUE(dst->invalidations.isEmpty());
}

TEST_F(NodeCoreTest, InvalidateAllRelaysThroughGraph)
{
	auto *src = AddNode<olive::MathNode>();
	auto *mid = AddNode<olive::MathNode>();
	auto *dst = AddNode<RecordingNode>();
	olive::Node::ConnectEdge(
		src, olive::NodeInput(mid, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(
		mid, olive::NodeInput(dst, RecordingNode::kTestInput));
	dst->invalidations.clear();

	const olive::rational kMin(INT_MIN);
	const olive::rational kMax(INT_MAX);

	// InvalidateAll propagates the full time range across multiple hops
	src->InvalidateAll(olive::MathNode::kParamAIn);
	ASSERT_EQ(dst->invalidations.size(), 1);
	EXPECT_EQ(dst->invalidations.first().from, RecordingNode::kTestInput);
	EXPECT_EQ(dst->invalidations.first().element, -1);
	EXPECT_EQ(dst->invalidations.first().range.in(), kMin);
	EXPECT_EQ(dst->invalidations.first().range.out(), kMax);

	// A zero-length range touches no caches but is still relayed
	src->InvalidateCache(
		olive::TimeRange(olive::rational(3), olive::rational(3)),
		olive::MathNode::kParamAIn, -1);
	ASSERT_EQ(dst->invalidations.size(), 2);
	EXPECT_EQ(dst->invalidations.at(1).range.in(), olive::rational(3));
	EXPECT_EQ(dst->invalidations.at(1).range.out(), olive::rational(3));

	// Disabled caches skip cache invalidation, but propagation continues
	EXPECT_TRUE(src->AreCachesEnabled());
	src->SetCachesEnabled(false);
	EXPECT_FALSE(src->AreCachesEnabled());
	src->InvalidateAll(olive::MathNode::kParamAIn);
	ASSERT_EQ(dst->invalidations.size(), 3);
	EXPECT_EQ(dst->invalidations.at(2).range.in(), kMin);
}

TEST_F(NodeCoreTest, KeyframeAddAndRemovalEmitSignals)
{
	auto *node = AddNode<olive::MathNode>();

	const olive::rational kMin(INT_MIN);
	const olive::rational kMax(INT_MAX);

	int enable_changed = 0;
	bool last_enabled = false;
	QObject::connect(node, &olive::Node::KeyframeEnableChanged,
					 [&enable_changed, &last_enabled](const olive::NodeInput &,
													 bool enabled) {
						 ++enable_changed;
						 last_enabled = enabled;
					 });
	int added = 0;
	int removed = 0;
	QObject::connect(node, &olive::Node::KeyframeAdded,
					 [&added](olive::NodeKeyframe *) { ++added; });
	QObject::connect(node, &olive::Node::KeyframeRemoved,
					 [&removed](olive::NodeKeyframe *) { ++removed; });
	QVector<olive::TimeRange> changed_ranges;
	QObject::connect(node, &olive::Node::ValueChanged,
					 [&changed_ranges](const olive::NodeInput &,
									   const olive::TimeRange &range) {
						 changed_ranges.append(range);
					 });

	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);
	EXPECT_EQ(enable_changed, 1);
	EXPECT_TRUE(last_enabled);

	// The first keyframe on a track invalidates the whole range
	auto *first = new olive::NodeKeyframe(
		olive::rational(5), 1.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	first->setParent(node);
	EXPECT_EQ(added, 1);
	ASSERT_EQ(changed_ranges.size(), 1);
	EXPECT_EQ(changed_ranges.first().in(), kMin);
	EXPECT_EQ(changed_ranges.first().out(), kMax);

	// A later keyframe only invalidates from the previous keyframe onward
	auto *second = new olive::NodeKeyframe(
		olive::rational(10), 2.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	second->setParent(node);
	EXPECT_EQ(added, 2);
	ASSERT_EQ(changed_ranges.size(), 2);
	EXPECT_EQ(changed_ranges.at(1).in(), olive::rational(5));
	EXPECT_EQ(changed_ranges.at(1).out(), kMax);

	// Removing the later keyframe invalidates from the remaining one onward
	second->setParent(nullptr);
	EXPECT_EQ(removed, 1);
	ASSERT_EQ(changed_ranges.size(), 3);
	EXPECT_EQ(changed_ranges.at(2).in(), olive::rational(5));
	EXPECT_EQ(changed_ranges.at(2).out(), kMax);
	delete second;

	// Removing the last keyframe invalidates everything again
	first->setParent(nullptr);
	EXPECT_EQ(removed, 2);
	ASSERT_EQ(changed_ranges.size(), 4);
	EXPECT_EQ(changed_ranges.at(3).in(), kMin);
	EXPECT_EQ(changed_ranges.at(3).out(), kMax);
	delete first;

	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());
}

TEST_F(NodeCoreTest, KeyframeTimeChangeResortsTrackAndEmits)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	auto *first = new olive::NodeKeyframe(
		olive::rational(0), 0.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	first->setParent(node);
	auto *second = new olive::NodeKeyframe(
		olive::rational(10), 10.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	second->setParent(node);

	const QVector<olive::NodeKeyframeTrack> &tracks =
		node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1);
	ASSERT_EQ(tracks.at(0).size(), 2);
	EXPECT_EQ(tracks.at(0).first(), first);
	EXPECT_EQ(node->GetEarliestKeyframe(olive::MathNode::kParamAIn), first);
	EXPECT_EQ(node->GetLatestKeyframe(olive::MathNode::kParamAIn), second);

	int time_changed = 0;
	olive::NodeKeyframe *last_changed = nullptr;
	QObject::connect(node, &olive::Node::KeyframeTimeChanged,
					 [&time_changed,
					  &last_changed](olive::NodeKeyframe *key) {
						 ++time_changed;
						 last_changed = key;
					 });

	// Moving the first keyframe past the second resorts the track
	first->set_time(olive::rational(20));
	EXPECT_EQ(time_changed, 1);
	EXPECT_EQ(last_changed, first);
	ASSERT_EQ(tracks.at(0).size(), 2);
	EXPECT_EQ(tracks.at(0).first(), second);
	EXPECT_EQ(tracks.at(0).last(), first);

	EXPECT_EQ(node->GetEarliestKeyframe(olive::MathNode::kParamAIn), second);
	EXPECT_EQ(node->GetLatestKeyframe(olive::MathNode::kParamAIn), first);
	EXPECT_EQ(node->GetClosestKeyframeBeforeTime(olive::MathNode::kParamAIn,
												 olive::rational(15)),
			  second);
	EXPECT_EQ(node->GetClosestKeyframeAfterTime(olive::MathNode::kParamAIn,
												olive::rational(15)),
			  first);
	EXPECT_TRUE(node->HasKeyframeAtTime(olive::MathNode::kParamAIn,
										olive::rational(20)));
	EXPECT_EQ(node->GetKeyframeAtTimeOnTrack(olive::MathNode::kParamAIn,
											 olive::rational(10), 0),
			  second);
	EXPECT_EQ(node->GetKeyframesAtTime(olive::MathNode::kParamAIn,
									   olive::rational(20))
				  .size(),
			  1);
}

TEST_F(NodeCoreTest, GetValueAtTimeUsesStandardValueWhenStatic)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetStandardValue(olive::MathNode::kParamAIn, 2.5);

	// A static input returns its standard value at any time
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(-100))
						 .toDouble(),
					 2.5);
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(0))
			.toDouble(),
		2.5);
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(100))
						 .toDouble(),
					 2.5);

	// Same result through the NodeInput convenience overload
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(
						 olive::NodeInput(node, olive::MathNode::kParamAIn),
						 olive::rational(3))
						 .toDouble(),
					 2.5);
}

TEST_F(NodeCoreTest, GetValueAtTimeInterpolatesLinearKeyframes)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	auto *first = new olive::NodeKeyframe(
		olive::rational(0), 0.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	first->setParent(node);
	auto *second = new olive::NodeKeyframe(
		olive::rational(10), 10.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	second->setParent(node);

	// Outside the keyed range the nearest keyframe value holds
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(-5))
						 .toDouble(),
					 0.0);
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(15))
						 .toDouble(),
					 10.0);

	// Exactly on a keyframe the value is exact
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(0))
			.toDouble(),
		0.0);
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(10))
			.toDouble(),
		10.0);

	// Between two linear keys the value interpolates linearly
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(5))
			.toDouble(),
		5.0);

	const olive::SplitValue split =
		node->GetSplitValueAtTime(olive::MathNode::kParamAIn,
								  olive::rational(5));
	ASSERT_EQ(split.size(), 1);
	EXPECT_DOUBLE_EQ(split.at(0).toDouble(), 5.0);
}

TEST_F(NodeCoreTest, GetValueAtTimeRespectsHoldKeyframes)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	auto *hold = new olive::NodeKeyframe(
		olive::rational(0), 1.0, olive::NodeKeyframe::kHold, 0, -1,
		olive::MathNode::kParamAIn);
	hold->setParent(node);
	auto *linear = new olive::NodeKeyframe(
		olive::rational(10), 3.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn);
	linear->setParent(node);

	// A hold keyframe keeps its value until the next keyframe's time
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(0))
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(5))
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(9))
			.toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		node->GetValueAtTime(olive::MathNode::kParamAIn, olive::rational(10))
			.toDouble(),
		3.0);
}

TEST_F(NodeCoreTest, CopyInputsCopiesValuesKeyframesLabelAndColor)
{
	auto *src = AddNode<olive::MathNode>();
	auto *dst = AddNode<olive::MathNode>();

	src->SetStandardValue(olive::MathNode::kParamAIn, 3.5);
	src->SetOperation(olive::MathNode::kOpMultiply);
	src->SetLabel(QStringLiteral("source label"));
	src->SetOverrideColor(2);
	src->SetValueHintForInput(
		olive::MathNode::kParamAIn,
		olive::Node::ValueHint({ olive::NodeValue::kVec2 }, 1,
							   QStringLiteral("hint")));

	src->SetInputIsKeyframing(olive::MathNode::kParamBIn, true);
	auto *key = new olive::NodeKeyframe(
		olive::rational(4), 8.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamBIn);
	key->setParent(src);

	olive::Node::CopyInputs(src, dst, false);

	EXPECT_DOUBLE_EQ(
		dst->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 3.5);
	EXPECT_EQ(dst->GetOperation(), olive::MathNode::kOpMultiply);
	EXPECT_EQ(dst->GetLabel(), QStringLiteral("source label"));
	EXPECT_EQ(dst->GetOverrideColor(), 2);
	EXPECT_EQ(dst->GetValueHintForInput(olive::MathNode::kParamAIn).tag(),
			  QStringLiteral("hint"));
	EXPECT_TRUE(dst->IsInputKeyframing(olive::MathNode::kParamBIn));

	const QVector<olive::NodeKeyframeTrack> &tracks =
		dst->GetKeyframeTracks(olive::MathNode::kParamBIn, -1);
	ASSERT_EQ(tracks.at(0).size(), 1);
	EXPECT_EQ(tracks.at(0).first()->time(), olive::rational(4));
	EXPECT_DOUBLE_EQ(tracks.at(0).first()->value().toDouble(), 8.0);
	// The copied keyframe belongs to the destination, not the source
	EXPECT_EQ(tracks.at(0).first()->parent(), dst);
	EXPECT_NE(tracks.at(0).first(), key);

	// Copied values are independent of the source
	src->SetStandardValue(olive::MathNode::kParamAIn, 9.0);
	EXPECT_DOUBLE_EQ(
		dst->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 3.5);
}

TEST_F(NodeCoreTest, CopyInputsCopiesConnectionsWhenRequested)
{
	auto *output = AddNode<olive::SolidGenerator>();
	auto *src = AddNode<olive::MathNode>();
	auto *dst = AddNode<olive::MathNode>();
	olive::Node::ConnectEdge(
		output, olive::NodeInput(src, olive::MathNode::kParamAIn));

	// Without connections requested, the destination stays unconnected
	olive::Node::CopyInputs(src, dst, false);
	EXPECT_EQ(dst->GetConnectedOutput(olive::MathNode::kParamAIn), nullptr);

	// With connections requested, the destination connects to the same output
	olive::Node::CopyInputs(src, dst, true);
	EXPECT_EQ(dst->GetConnectedOutput(olive::MathNode::kParamAIn), output);
}

TEST_F(NodeCoreTest, CopyInputsCopiesArrayElements)
{
	auto *src = AddNode<olive::TextGeneratorV3>();
	auto *dst = AddNode<olive::TextGeneratorV3>();

	src->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 2);
	src->SetStandardValue(
		olive::NodeInput(src, olive::TextGeneratorV3::kArgsInput, 0),
		QStringLiteral("first"));
	src->SetStandardValue(
		olive::NodeInput(src, olive::TextGeneratorV3::kArgsInput, 1),
		QStringLiteral("second"));
	src->SetInputIsKeyframing(olive::TextGeneratorV3::kArgsInput, true, 1);
	auto *key = new olive::NodeKeyframe(
		olive::rational(2), QStringLiteral("keyed"), olive::NodeKeyframe::kLinear,
		0, 1, olive::TextGeneratorV3::kArgsInput);
	key->setParent(src);

	olive::Node::CopyInputs(src, dst, false);

	EXPECT_EQ(dst->InputArraySize(olive::TextGeneratorV3::kArgsInput), 2);
	EXPECT_EQ(dst->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput, 0)
				  .at(0)
				  .toString(),
			  QStringLiteral("first"));
	EXPECT_EQ(dst->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput, 1)
				  .at(0)
				  .toString(),
			  QStringLiteral("second"));
	EXPECT_TRUE(dst->IsInputKeyframing(olive::TextGeneratorV3::kArgsInput, 1));
	EXPECT_FALSE(dst->IsInputKeyframing(olive::TextGeneratorV3::kArgsInput, 0));
	ASSERT_EQ(dst->GetKeyframeTracks(olive::TextGeneratorV3::kArgsInput, 1)
				  .at(0)
				  .size(),
			  1);
	EXPECT_EQ(dst->GetKeyframeTracks(olive::TextGeneratorV3::kArgsInput, 1)
				  .at(0)
				  .first()
				  ->value()
				  .toString(),
			  QStringLiteral("keyed"));
}

TEST_F(NodeCoreTest, CopyDependencyGraphClonesAndReconnects)
{
	auto *solid = AddNode<olive::SolidGenerator>();
	auto *math = AddNode<olive::MathNode>();
	math->SetStandardValue(olive::MathNode::kParamAIn, 6.0);
	olive::Node::ConnectEdge(
		solid, olive::NodeInput(math, olive::MathNode::kParamAIn));
	math->SetValueHintForInput(olive::MathNode::kParamAIn,
							   olive::Node::ValueHint(QStringLiteral("tagged")));

	const QVector<olive::Node *> copies =
		olive::Node::CopyDependencyGraph({ solid, math }, nullptr);
	ASSERT_EQ(copies.size(), 2);

	olive::Node *solid_copy = copies.at(0);
	olive::Node *math_copy = copies.at(1);
	EXPECT_NE(solid_copy, solid);
	EXPECT_NE(math_copy, math);
	EXPECT_EQ(solid_copy->id(), solid->id());
	EXPECT_EQ(math_copy->id(), math->id());
	EXPECT_EQ(solid_copy->project(), project_.get());
	EXPECT_EQ(math_copy->project(), project_.get());

	// The clone is wired to the cloned upstream node, not the original
	EXPECT_EQ(math_copy->GetConnectedOutput(olive::MathNode::kParamAIn),
			  solid_copy);
	EXPECT_EQ(math->GetConnectedOutput(olive::MathNode::kParamAIn), solid);

	// ...and carries the source's values and hints
	EXPECT_DOUBLE_EQ(
		math_copy->GetStandardValue(olive::MathNode::kParamAIn).toDouble(),
		6.0);
	EXPECT_EQ(math_copy->GetValueHintForInput(olive::MathNode::kParamAIn).tag(),
			  QStringLiteral("tagged"));
}
