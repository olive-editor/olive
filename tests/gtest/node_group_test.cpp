#include <gtest/gtest.h>

#include <memory>

#include <QSignalSpy>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "node/group/group.h"
#include "node/math/math/math.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "node/serializeddata.h"
#include "node/value.h"

class NodeGroupTest : public ::testing::Test {
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

	// AddInputPassthrough()/SetOutputPassthrough() assert that the inner node
	// is part of the group's context, so tests always place it there first
	olive::NodeGroup *AddGroupWithInnerMath(olive::MathNode **math)
	{
		olive::NodeGroup *group = AddNode<olive::NodeGroup>();
		*math = AddNode<olive::MathNode>();
		group->SetNodePositionInContext(*math, olive::Node::Position());
		return group;
	}

	std::unique_ptr<olive::Project> project_;
};

TEST_F(NodeGroupTest, MetadataIsCorrect)
{
	olive::NodeGroup group;

	EXPECT_EQ(group.id(), QStringLiteral("org.olivevideoeditor.Olive.group"));
	EXPECT_EQ(group.Name(), QStringLiteral("Group"));
	EXPECT_TRUE(group.Category().contains(olive::Node::kCategoryUnknown));
	EXPECT_FALSE(group.Description().isEmpty());
	EXPECT_TRUE(group.GetFlags() & olive::Node::kDontShowInCreateMenu);

	// A fresh group has no passthroughs of either kind
	EXPECT_EQ(group.GetOutputPassthrough(), nullptr);
	EXPECT_TRUE(group.GetInputPassthroughs().isEmpty());
}

TEST_F(NodeGroupTest, AddInputPassthroughRegistersMirroredInput)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	QSignalSpy added_spy(group, &olive::NodeGroup::InputPassthroughAdded);
	ASSERT_TRUE(added_spy.isValid());

	const olive::NodeInput input(math, olive::MathNode::kParamAIn);
	const QString id = group->AddInputPassthrough(input);

	// The first passthrough of an input reuses the inner input's ID
	EXPECT_EQ(id, olive::MathNode::kParamAIn);
	EXPECT_TRUE(group->HasInputWithID(id));

	// The group input mirrors the inner input's type, default and flags
	EXPECT_EQ(group->GetInputDataType(id), olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(group->GetDefaultValue(id).toDouble(), 0.0);
	EXPECT_EQ(group->GetInputFlags(id).value(),
			  math->GetInputFlags(olive::MathNode::kParamAIn).value());

	// The passthrough is registered for lookup in both directions
	ASSERT_EQ(group->GetInputPassthroughs().size(), 1);
	EXPECT_EQ(group->GetInputPassthroughs().first().first, id);
	EXPECT_EQ(group->GetInputPassthroughs().first().second, input);
	EXPECT_TRUE(group->ContainsInputPassthrough(input));
	EXPECT_FALSE(group->ContainsInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamBIn)));
	EXPECT_EQ(group->GetIDOfPassthrough(input), id);
	EXPECT_EQ(group->GetInputFromID(id), input);

	// Unknown lookups return empty/invalid results
	EXPECT_TRUE(group->GetIDOfPassthrough(
					 olive::NodeInput(math, olive::MathNode::kParamBIn))
					.isEmpty());
	EXPECT_FALSE(
		group->GetInputFromID(QStringLiteral("no_such_input")).IsValid());

	ASSERT_EQ(added_spy.count(), 1);
	EXPECT_EQ(added_spy.takeFirst().at(1).value<olive::NodeInput>(), input);
}

TEST_F(NodeGroupTest, AddInputPassthroughIsIdempotentForSameInput)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	const olive::NodeInput input(math, olive::MathNode::kParamAIn);
	const QString first = group->AddInputPassthrough(input);

	QSignalSpy added_spy(group, &olive::NodeGroup::InputPassthroughAdded);
	const QString second = group->AddInputPassthrough(input);

	// Passing the same input through twice returns the existing ID
	EXPECT_EQ(first, second);
	EXPECT_EQ(group->GetInputPassthroughs().size(), 1);
	EXPECT_EQ(added_spy.count(), 0);
}

TEST_F(NodeGroupTest, AddInputPassthroughGeneratesUniqueIdForDuplicateInputId)
{
	auto *math_a = AddNode<olive::MathNode>();
	auto *math_b = AddNode<olive::MathNode>();
	auto *group = AddNode<olive::NodeGroup>();
	group->SetNodePositionInContext(math_a, olive::Node::Position());
	group->SetNodePositionInContext(math_b, olive::Node::Position());

	const QString id_a = group->AddInputPassthrough(
		olive::NodeInput(math_a, olive::MathNode::kParamAIn));
	EXPECT_EQ(id_a, olive::MathNode::kParamAIn);

	// A second passthrough of the same input ID (on a different node) must
	// not collide with the first; the suffix is derived from the input ID
	const QString id_b = group->AddInputPassthrough(
		olive::NodeInput(math_b, olive::MathNode::kParamAIn));

	EXPECT_NE(id_a, id_b);
	EXPECT_EQ(id_b, QStringLiteral("param_a_in_2"));

	ASSERT_EQ(group->GetInputPassthroughs().size(), 2);
	EXPECT_TRUE(group->HasInputWithID(id_b));
	EXPECT_EQ(group->GetInputFromID(id_b),
			  olive::NodeInput(math_b, olive::MathNode::kParamAIn));
	EXPECT_EQ(group->GetIDOfPassthrough(
				  olive::NodeInput(math_a, olive::MathNode::kParamAIn)),
			  id_a);
}

TEST_F(NodeGroupTest, AddInputPassthroughHonorsForcedIdAndMirrorsFlags)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	// kMethodIn is declared with kInputFlagNotConnectable |
	// kInputFlagNotKeyframable, exercising the flag mirroring
	const QString id = group->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kMethodIn),
		QStringLiteral("forced_method"));

	EXPECT_EQ(id, QStringLiteral("forced_method"));
	EXPECT_TRUE(group->HasInputWithID(id));
	EXPECT_EQ(group->GetInputDataType(id), olive::NodeValue::kCombo);
	EXPECT_EQ(group->GetInputFlags(id).value(),
			  math->GetInputFlags(olive::MathNode::kMethodIn).value());
	EXPECT_FALSE(group->IsInputConnectable(id));
	EXPECT_FALSE(group->IsInputKeyframable(id));
	EXPECT_EQ(group->GetInputFromID(id),
			  olive::NodeInput(math, olive::MathNode::kMethodIn));
}

TEST_F(NodeGroupTest, RemoveInputPassthroughMirrorsInputDeletion)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	const olive::NodeInput input(math, olive::MathNode::kParamAIn);
	const QString id = group->AddInputPassthrough(input);

	QSignalSpy removed_spy(group, &olive::NodeGroup::InputPassthroughRemoved);
	ASSERT_TRUE(removed_spy.isValid());

	group->RemoveInputPassthrough(input);

	EXPECT_EQ(removed_spy.count(), 1);
	EXPECT_TRUE(group->GetInputPassthroughs().isEmpty());
	EXPECT_FALSE(group->ContainsInputPassthrough(input));
	EXPECT_FALSE(group->HasInputWithID(id));
	EXPECT_TRUE(group->GetIDOfPassthrough(input).isEmpty());
	EXPECT_FALSE(group->GetInputFromID(id).IsValid());
}

TEST_F(NodeGroupTest, RemoveInputPassthroughIgnoresUnknownInput)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	const QString id = group->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn));

	QSignalSpy removed_spy(group, &olive::NodeGroup::InputPassthroughRemoved);

	// An input that was never passed through must be a harmless no-op
	group->RemoveInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamBIn));
	group->RemoveInputPassthrough(olive::NodeInput());

	EXPECT_EQ(removed_spy.count(), 0);
	EXPECT_EQ(group->GetInputPassthroughs().size(), 1);
	EXPECT_TRUE(group->HasInputWithID(id));
}

TEST_F(NodeGroupTest, SetOutputPassthroughUpdatesAndEmits)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	ASSERT_EQ(group->GetOutputPassthrough(), nullptr);

	QSignalSpy output_spy(group, &olive::NodeGroup::OutputPassthroughChanged);
	ASSERT_TRUE(output_spy.isValid());

	group->SetOutputPassthrough(math);
	EXPECT_EQ(group->GetOutputPassthrough(), math);
	EXPECT_EQ(output_spy.count(), 1);

	// Clearing the passthrough is allowed
	group->SetOutputPassthrough(nullptr);
	EXPECT_EQ(group->GetOutputPassthrough(), nullptr);
	EXPECT_EQ(output_spy.count(), 2);
}

TEST_F(NodeGroupTest, PassthroughInputAcceptsExternalEdges)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	const QString id = group->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn));

	// The mirrored input is a real input on the group and can be connected
	// from nodes outside the group
	auto *external = AddNode<olive::MathNode>();
	const olive::NodeInput group_input(group, id);
	EXPECT_FALSE(group_input.IsConnected());

	olive::Node::ConnectEdge(external, group_input);
	EXPECT_TRUE(group_input.IsConnected());
	EXPECT_EQ(group_input.GetConnectedOutput(), external);
	EXPECT_EQ(external->output_connections().size(), 1);

	olive::Node::DisconnectEdge(external, group_input);
	EXPECT_FALSE(group_input.IsConnected());
	EXPECT_TRUE(external->output_connections().empty());
}

TEST_F(NodeGroupTest, GetInputNameFallsThroughToInnerNode)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	math->Retranslate();

	const QString id = group->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn));

	// Without an override the name comes from the inner node's input
	EXPECT_EQ(group->GetInputName(id), QStringLiteral("Value"));

	// An explicit override takes precedence
	group->SetInputName(id, QStringLiteral("Custom Name"));
	EXPECT_EQ(group->GetInputName(id), QStringLiteral("Custom Name"));

	// Clearing the override restores the fall-through
	group->SetInputName(id, QString());
	EXPECT_EQ(group->GetInputName(id), QStringLiteral("Value"));
}

TEST_F(NodeGroupTest, GetInputNameResolvesThroughNestedGroups)
{
	auto *math = AddNode<olive::MathNode>();
	math->Retranslate();

	auto *inner = AddNode<olive::NodeGroup>();
	inner->SetNodePositionInContext(math, olive::Node::Position());
	const QString inner_id = inner->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn));

	auto *outer = AddNode<olive::NodeGroup>();
	outer->SetNodePositionInContext(inner, olive::Node::Position());
	const QString outer_id = outer->AddInputPassthrough(
		olive::NodeInput(inner, inner_id));

	// The outer group asks the inner group, which asks the math node
	EXPECT_EQ(outer->GetInputName(outer_id), QStringLiteral("Value"));
}

TEST_F(NodeGroupTest, ResolveInputUnwrapsNestedGroups)
{
	auto *math = AddNode<olive::MathNode>();

	auto *inner = AddNode<olive::NodeGroup>();
	inner->SetNodePositionInContext(math, olive::Node::Position());
	const QString inner_id = inner->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn));

	auto *outer = AddNode<olive::NodeGroup>();
	outer->SetNodePositionInContext(inner, olive::Node::Position());
	const QString outer_id = outer->AddInputPassthrough(
		olive::NodeInput(inner, inner_id));

	// An input on the outer group resolves to the innermost real input
	const olive::NodeInput resolved = olive::NodeGroup::ResolveInput(
		olive::NodeInput(outer, outer_id));
	EXPECT_EQ(resolved.node(), math);
	EXPECT_EQ(resolved.input(), olive::MathNode::kParamAIn);
	EXPECT_EQ(resolved.element(), -1);

	// Inputs on regular nodes are returned unchanged
	const olive::NodeInput plain(math, olive::MathNode::kParamBIn);
	EXPECT_EQ(olive::NodeGroup::ResolveInput(plain), plain);
}

TEST_F(NodeGroupTest, GetInnerRejectsNonGroupAndUnknownInputs)
{
	auto *math = AddNode<olive::MathNode>();

	auto *group = AddNode<olive::NodeGroup>();
	group->SetNodePositionInContext(math, olive::Node::Position());
	const QString id = group->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn));

	// A node that is not a group has no inner input
	olive::NodeInput non_group(math, olive::MathNode::kParamAIn);
	EXPECT_FALSE(olive::NodeGroup::GetInner(&non_group));
	EXPECT_EQ(non_group.node(), math);

	// An input ID that is not a passthrough is left unchanged
	olive::NodeInput unknown(group, QStringLiteral("does_not_exist"));
	EXPECT_FALSE(olive::NodeGroup::GetInner(&unknown));
	EXPECT_EQ(unknown.node(), group);

	// One level of passthrough resolves to the inner node's input
	olive::NodeInput passthrough(group, id);
	EXPECT_TRUE(olive::NodeGroup::GetInner(&passthrough));
	EXPECT_EQ(passthrough.node(), math);
	EXPECT_EQ(passthrough.input(), olive::MathNode::kParamAIn);
}

TEST_F(NodeGroupTest, RetranslateRetranslatesContextNodes)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	ASSERT_TRUE(math->GetInputName(olive::MathNode::kParamAIn).isEmpty());

	group->Retranslate();

	// The group retranslates itself and every node in its context
	EXPECT_EQ(group->GetInputName(olive::Node::kEnabledInput),
			  QStringLiteral("Enabled"));
	EXPECT_EQ(math->GetInputName(olive::MathNode::kParamAIn),
			  QStringLiteral("Value"));
}

TEST_F(NodeGroupTest, SaveCustomWritesInputPassthroughs)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	const QString id = group->AddInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn),
		QStringLiteral("pt_float"));
	group->SetInputName(id, QStringLiteral("Custom Name"));
	group->SetDefaultValue(id, 2.5);
	group->SetInputFlag(id, olive::kInputFlagHidden);
	group->SetInputProperty(id, QStringLiteral("mykey"),
							QStringLiteral("myvalue"));
	group->SetOutputPassthrough(math);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	group->SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	const QString ptr = QString::number(reinterpret_cast<quintptr>(math));

	EXPECT_TRUE(xml.contains(QStringLiteral("<inputpassthroughs>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<node>%1</node>").arg(ptr)));
	EXPECT_TRUE(xml.contains(
		QStringLiteral("<input>%1</input>").arg(olive::MathNode::kParamAIn)));
	EXPECT_TRUE(xml.contains(QStringLiteral("<element>-1</element>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<id>pt_float</id>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<name>Custom Name</name>")));
	// 8 == kInputFlagHidden; only flags differing from the inner input are
	// saved, and the inner input has kInputFlagNormal
	EXPECT_TRUE(xml.contains(QStringLiteral("<flags>8</flags>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<type>float</type>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<default>2.5</default>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<key>mykey</key>")));
	EXPECT_TRUE(xml.contains(QStringLiteral("<value>myvalue</value>")));
	EXPECT_TRUE(xml.contains(
		QStringLiteral("<outputpassthrough>%1</outputpassthrough>")
			.arg(ptr)));
}

TEST_F(NodeGroupTest, LoadCustomCollectsGroupLinks)
{
	auto *group = AddNode<olive::NodeGroup>();

	const QString xml = QStringLiteral(
		"<custom>"
		"<inputpassthroughs>"
		"<inputpassthrough>"
		"<node>12345</node>"
		"<input>param_a_in</input>"
		"<element>-1</element>"
		"<id>pt_a</id>"
		"<name>Custom A</name>"
		"<flags>8</flags>"
		"<type>float</type>"
		"<default>2.5</default>"
		"<properties>"
		"<property>"
		"<key>mykey</key>"
		"<value>myvalue</value>"
		"</property>"
		"</properties>"
		"</inputpassthrough>"
		"</inputpassthroughs>"
		"<outputpassthrough>12345</outputpassthrough>"
		"</custom>");

	olive::SerializedData data;
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));
	EXPECT_TRUE(group->LoadCustom(&reader, &data));

	// Loading only records the links; resolution happens in PostLoadEvent
	ASSERT_EQ(data.group_input_links.size(), 1);
	const olive::SerializedData::GroupLink &link =
		data.group_input_links.first();
	EXPECT_EQ(link.group, group);
	EXPECT_EQ(link.passthrough_id, QStringLiteral("pt_a"));
	EXPECT_EQ(link.input_node, static_cast<quintptr>(12345));
	EXPECT_EQ(link.input_id, QStringLiteral("param_a_in"));
	EXPECT_EQ(link.input_element, -1);
	EXPECT_EQ(link.custom_name, QStringLiteral("Custom A"));
	EXPECT_EQ(link.custom_flags.value(), uint64_t(8));
	EXPECT_EQ(link.data_type, olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(link.default_val.toDouble(), 2.5);
	EXPECT_EQ(link.custom_properties.value(QStringLiteral("mykey"))
				  .toString(),
			  QStringLiteral("myvalue"));

	ASSERT_EQ(data.group_output_links.size(), 1);
	EXPECT_EQ(data.group_output_links.value(group),
			  static_cast<quintptr>(12345));

	// Nothing has been applied to the group yet
	EXPECT_TRUE(group->GetInputPassthroughs().isEmpty());
	EXPECT_EQ(group->GetOutputPassthrough(), nullptr);
}

TEST_F(NodeGroupTest, PostLoadEventRecreatesPassthroughs)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);

	const QString xml = QStringLiteral(
		"<custom>"
		"<inputpassthroughs>"
		"<inputpassthrough>"
		"<node>12345</node>"
		"<input>param_a_in</input>"
		"<element>-1</element>"
		"<id>pt_a</id>"
		"<name>Custom A</name>"
		"<flags>8</flags>"
		"<type>float</type>"
		"<default>2.5</default>"
		"<properties>"
		"<property>"
		"<key>mykey</key>"
		"<value>myvalue</value>"
		"</property>"
		"</properties>"
		"</inputpassthrough>"
		"</inputpassthroughs>"
		"<outputpassthrough>12345</outputpassthrough>"
		"</custom>");

	olive::SerializedData data;
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_TRUE(group->LoadCustom(&reader, &data));

	// Point the serialized node references at the real inner node
	data.node_ptrs.insert(static_cast<quintptr>(12345), math);
	group->PostLoadEvent(&data);

	// The passthrough input is recreated with all its serialized overrides
	ASSERT_TRUE(group->HasInputWithID(QStringLiteral("pt_a")));
	EXPECT_TRUE(group->ContainsInputPassthrough(
		olive::NodeInput(math, olive::MathNode::kParamAIn)));
	EXPECT_EQ(group->GetInputDataType(QStringLiteral("pt_a")),
			  olive::NodeValue::kFloat);
	EXPECT_EQ(group->GetInputName(QStringLiteral("pt_a")),
			  QStringLiteral("Custom A"));
	EXPECT_TRUE(group->IsInputHidden(QStringLiteral("pt_a")));
	EXPECT_DOUBLE_EQ(
		group->GetDefaultValue(QStringLiteral("pt_a")).toDouble(), 2.5);
	EXPECT_EQ(group
				  ->GetInputProperty(QStringLiteral("pt_a"),
									 QStringLiteral("mykey"))
				  .toString(),
			  QStringLiteral("myvalue"));

	EXPECT_EQ(group->GetOutputPassthrough(), math);
}

TEST_F(NodeGroupTest, SaveLoadRoundTripPreservesPassthroughs)
{
	olive::MathNode *math_a;
	olive::NodeGroup *group_a = AddGroupWithInnerMath(&math_a);

	const QString id = group_a->AddInputPassthrough(
		olive::NodeInput(math_a, olive::MathNode::kParamAIn),
		QStringLiteral("pt_roundtrip"));
	group_a->SetInputName(id, QStringLiteral("Original Name"));
	group_a->SetOutputPassthrough(math_a);

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	group_a->SaveCustom(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();

	// Load into a fresh group whose inner node replaces the original one
	olive::MathNode *math_b;
	olive::NodeGroup *group_b = AddGroupWithInnerMath(&math_b);

	olive::SerializedData data;
	data.node_ptrs.insert(reinterpret_cast<quintptr>(math_a), math_b);

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));
	ASSERT_TRUE(group_b->LoadCustom(&reader, &data));
	group_b->PostLoadEvent(&data);

	ASSERT_EQ(group_b->GetInputPassthroughs().size(), 1);
	EXPECT_EQ(group_b->GetInputPassthroughs().first().first, id);
	EXPECT_EQ(group_b->GetInputPassthroughs().first().second.node(), math_b);
	EXPECT_EQ(group_b->GetInputPassthroughs().first().second.input(),
			  olive::MathNode::kParamAIn);
	EXPECT_EQ(group_b->GetInputName(id), QStringLiteral("Original Name"));
	EXPECT_EQ(group_b->GetOutputPassthrough(), math_b);
}

TEST_F(NodeGroupTest, AddInputPassthroughCommandAddsAndRemoves)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	const olive::NodeInput input(math, olive::MathNode::kParamAIn);

	olive::NodeGroupAddInputPassthrough cmd(group, input);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_EQ(group->GetInputPassthroughs().size(), 1);
	EXPECT_TRUE(group->ContainsInputPassthrough(input));

	cmd.undo_now();
	EXPECT_TRUE(group->GetInputPassthroughs().isEmpty());
	EXPECT_FALSE(group->HasInputWithID(olive::MathNode::kParamAIn));
}

TEST_F(NodeGroupTest, AddInputPassthroughCommandNoOpWhenAlreadyPresent)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	const olive::NodeInput input(math, olive::MathNode::kParamAIn);
	group->AddInputPassthrough(input);
	ASSERT_EQ(group->GetInputPassthroughs().size(), 1);

	olive::NodeGroupAddInputPassthrough cmd(group, input);

	// Redo must not add a duplicate when the passthrough already exists
	cmd.redo_now();
	EXPECT_EQ(group->GetInputPassthroughs().size(), 1);

	// And undo must not remove the pre-existing passthrough
	cmd.undo_now();
	EXPECT_EQ(group->GetInputPassthroughs().size(), 1);
	EXPECT_TRUE(group->ContainsInputPassthrough(input));
}

TEST_F(NodeGroupTest, SetOutputPassthroughCommandRestoresPreviousOutput)
{
	olive::MathNode *math;
	olive::NodeGroup *group = AddGroupWithInnerMath(&math);
	auto *other = AddNode<olive::MathNode>();
	group->SetNodePositionInContext(other, olive::Node::Position());

	olive::NodeGroupSetOutputPassthrough cmd(group, math);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(group->GetOutputPassthrough(), math);

	// Replacing the output passthrough restores the previous one on undo
	olive::NodeGroupSetOutputPassthrough replace_cmd(group, other);
	replace_cmd.redo_now();
	EXPECT_EQ(group->GetOutputPassthrough(), other);
	replace_cmd.undo_now();
	EXPECT_EQ(group->GetOutputPassthrough(), math);

	cmd.undo_now();
	EXPECT_EQ(group->GetOutputPassthrough(), nullptr);
}
