#include <gtest/gtest.h>

#include <memory>

#include <QCoreApplication>
#include <QPointF>
#include <QThread>

#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv3.h"
#include "node/inputimmediate.h"
#include "node/keyframe.h"
#include "node/math/math/math.h"
#include "node/nodeundo.h"
#include "node/project.h"
#include "node/project/folder/folder.h"

class NodeUndoTest : public ::testing::Test {
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

TEST_F(NodeUndoTest, AddCommandAddsAndRemovesNodeFromProject)
{
	auto *node = new olive::MathNode(); // Intentionally parentless

	olive::NodeAddCommand cmd(project_.get(), node);

	// The constructor takes ownership of the node without adding it to the graph
	EXPECT_FALSE(project_->nodes().contains(node));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.PushToThread(QCoreApplication::instance()->thread());

	cmd.redo_now();
	EXPECT_TRUE(project_->nodes().contains(node));
	EXPECT_EQ(node->project(), project_.get());

	cmd.undo_now();
	EXPECT_FALSE(project_->nodes().contains(node));
	EXPECT_EQ(node->project(), nullptr);
	// cmd's destructor disposes of the unparented node
}

TEST_F(NodeUndoTest, RemoveAndDisconnectCommandRestoresGraphState)
{
	auto *src = AddNode<olive::SolidGenerator>();
	auto *mid = AddNode<olive::MathNode>();
	auto *dst = AddNode<olive::MathNode>();
	auto *link_peer = AddNode<olive::SolidGenerator>();
	auto *context = AddNode<olive::Folder>();

	olive::Node::ConnectEdge(
		src, olive::NodeInput(mid, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(
		mid, olive::NodeInput(dst, olive::MathNode::kParamBIn));
	olive::Node::Link(mid, link_peer);
	context->SetNodePositionInContext(
		mid, olive::Node::Position(QPointF(3.0, 4.0), true));

	olive::NodeRemoveAndDisconnectCommand cmd(mid);
	cmd.redo_now();

	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());
	EXPECT_EQ(mid->project(), nullptr);
	EXPECT_FALSE(project_->nodes().contains(mid));
	EXPECT_TRUE(mid->input_connections().empty());
	EXPECT_TRUE(mid->output_connections().empty());
	EXPECT_TRUE(src->output_connections().empty());
	EXPECT_TRUE(dst->input_connections().empty());
	EXPECT_FALSE(context->ContextContainsNode(mid));
	EXPECT_FALSE(mid->HasLinks());

	cmd.undo_now();

	EXPECT_EQ(mid->project(), project_.get());
	EXPECT_TRUE(project_->nodes().contains(mid));
	ASSERT_EQ(src->output_connections().size(), 1);
	EXPECT_EQ(src->output_connections().front().second,
			  olive::NodeInput(mid, olive::MathNode::kParamAIn));
	EXPECT_EQ(dst->input_connections().at(
				  olive::NodeInput(dst, olive::MathNode::kParamBIn)),
			  mid);
	ASSERT_TRUE(context->ContextContainsNode(mid));
	EXPECT_EQ(context->GetNodePositionInContext(mid), QPointF(3.0, 4.0));
	EXPECT_TRUE(olive::Node::AreLinked(mid, link_peer));
}

TEST_F(NodeUndoTest, RemoveWithExclusiveDependenciesRemovesUpstreamChain)
{
	auto *src = AddNode<olive::SolidGenerator>();
	auto *dep = AddNode<olive::MathNode>();
	auto *node = AddNode<olive::MathNode>();

	olive::Node::ConnectEdge(
		src, olive::NodeInput(dep, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(
		dep, olive::NodeInput(node, olive::MathNode::kParamAIn));

	olive::NodeRemoveWithExclusiveDependenciesAndDisconnect cmd(node);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();

	// The node and all of its non-item upstream dependencies are removed
	EXPECT_EQ(node->project(), nullptr);
	EXPECT_EQ(dep->project(), nullptr);
	EXPECT_EQ(src->project(), nullptr);
	EXPECT_TRUE(node->input_connections().empty());
	EXPECT_TRUE(dep->input_connections().empty());
	EXPECT_TRUE(src->output_connections().empty());
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.undo_now();

	EXPECT_EQ(node->project(), project_.get());
	EXPECT_EQ(dep->project(), project_.get());
	EXPECT_EQ(src->project(), project_.get());
	EXPECT_EQ(node->input_connections().at(
				  olive::NodeInput(node, olive::MathNode::kParamAIn)),
			  dep);
	EXPECT_EQ(dep->input_connections().at(
				  olive::NodeInput(dep, olive::MathNode::kParamAIn)),
			  src);
}

TEST_F(NodeUndoTest, EdgeAddCommandConnectsAndDisconnects)
{
	auto *output = AddNode<olive::SolidGenerator>();
	auto *input_node = AddNode<olive::MathNode>();
	const olive::NodeInput input(input_node, olive::MathNode::kParamAIn);

	olive::NodeEdgeAddCommand cmd(output, input);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_TRUE(input.IsConnected());
	EXPECT_EQ(input.GetConnectedOutput(), output);
	EXPECT_EQ(output->output_connections().size(), 1);

	cmd.undo_now();
	EXPECT_FALSE(input.IsConnected());
	EXPECT_TRUE(output->output_connections().empty());
}

TEST_F(NodeUndoTest, EdgeAddCommandReplacesExistingConnection)
{
	auto *first = AddNode<olive::SolidGenerator>();
	auto *second = AddNode<olive::MathNode>();
	auto *input_node = AddNode<olive::MathNode>();
	const olive::NodeInput input(input_node, olive::MathNode::kParamAIn);

	olive::Node::ConnectEdge(first, input);
	ASSERT_EQ(input.GetConnectedOutput(), first);

	olive::NodeEdgeAddCommand cmd(second, input);
	cmd.redo_now();

	// The previous edge must be disconnected before the new one is made
	EXPECT_EQ(input.GetConnectedOutput(), second);
	EXPECT_TRUE(first->output_connections().empty());

	cmd.undo_now();

	// Undoing must restore the connection that was replaced
	EXPECT_EQ(input.GetConnectedOutput(), first);
	EXPECT_TRUE(second->output_connections().empty());
}

TEST_F(NodeUndoTest, EdgeRemoveCommandDisconnectsAndReconnects)
{
	auto *output = AddNode<olive::SolidGenerator>();
	auto *input_node = AddNode<olive::MathNode>();
	const olive::NodeInput input(input_node, olive::MathNode::kParamBIn);

	olive::Node::ConnectEdge(output, input);
	ASSERT_TRUE(input.IsConnected());

	olive::NodeEdgeRemoveCommand cmd(output, input);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_FALSE(input.IsConnected());
	EXPECT_TRUE(output->output_connections().empty());

	cmd.undo_now();
	EXPECT_TRUE(input.IsConnected());
	EXPECT_EQ(input.GetConnectedOutput(), output);
}

TEST_F(NodeUndoTest, SetPositionCommandAddsNodeToContext)
{
	auto *node = AddNode<olive::MathNode>();
	auto *context = AddNode<olive::Folder>();
	ASSERT_FALSE(context->ContextContainsNode(node));

	olive::NodeSetPositionCommand cmd(
		node, context, olive::Node::Position(QPointF(10.0, 20.0), true));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	ASSERT_TRUE(context->ContextContainsNode(node));
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(10.0, 20.0));
	EXPECT_TRUE(context->IsNodeExpandedInContext(node));

	cmd.undo_now();
	EXPECT_FALSE(context->ContextContainsNode(node));
}

TEST_F(NodeUndoTest, SetPositionCommandRestoresPreviousPosition)
{
	auto *node = AddNode<olive::MathNode>();
	auto *context = AddNode<olive::Folder>();
	context->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(1.0, 2.0)));

	olive::NodeSetPositionCommand cmd(
		node, context, olive::Node::Position(QPointF(30.0, 40.0)));

	cmd.redo_now();
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(30.0, 40.0));

	cmd.undo_now();
	ASSERT_TRUE(context->ContextContainsNode(node));
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(1.0, 2.0));
}

TEST_F(NodeUndoTest, SetPositionAndDependenciesRecursivelyMovesNode)
{
	auto *dep = AddNode<olive::MathNode>();
	auto *node = AddNode<olive::MathNode>();
	auto *context = AddNode<olive::Folder>();

	olive::Node::ConnectEdge(
		dep, olive::NodeInput(node, olive::MathNode::kParamAIn));

	context->SetNodePositionInContext(
		dep, olive::Node::Position(QPointF(1.0, 1.0)));
	context->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(2.0, 3.0)));

	olive::NodeSetPositionAndDependenciesRecursivelyCommand cmd(
		node, context, olive::Node::Position(QPointF(8.0, 9.0)));

	cmd.redo_now();
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(8.0, 9.0));
	// The dependency moves by the same delta as the node
	EXPECT_EQ(context->GetNodePositionInContext(dep), QPointF(7.0, 7.0));

	cmd.undo_now();
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(2.0, 3.0));
	EXPECT_EQ(context->GetNodePositionInContext(dep), QPointF(1.0, 1.0));
}

TEST_F(NodeUndoTest, RemovePositionFromContextCommandRestoresPosition)
{
	auto *node = AddNode<olive::MathNode>();
	auto *context = AddNode<olive::Folder>();
	context->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(5.0, 6.0)));

	olive::NodeRemovePositionFromContextCommand cmd(node, context);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_FALSE(context->ContextContainsNode(node));

	cmd.undo_now();
	ASSERT_TRUE(context->ContextContainsNode(node));
	EXPECT_EQ(context->GetNodePositionInContext(node), QPointF(5.0, 6.0));
}

TEST_F(NodeUndoTest, RemovePositionFromContextCommandNoOpWhenAbsent)
{
	auto *node = AddNode<olive::MathNode>();
	auto *context = AddNode<olive::Folder>();

	olive::NodeRemovePositionFromContextCommand cmd(node, context);

	cmd.redo_now();
	EXPECT_FALSE(context->ContextContainsNode(node));

	cmd.undo_now();
	EXPECT_FALSE(context->ContextContainsNode(node));
}

TEST_F(NodeUndoTest, RemovePositionFromAllContextsCommandRestoresAll)
{
	auto *node = AddNode<olive::MathNode>();
	auto *ctx_a = AddNode<olive::Folder>();
	auto *ctx_b = AddNode<olive::Folder>();
	ctx_a->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(1.0, 1.0)));
	ctx_b->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(2.0, 2.0)));

	olive::NodeRemovePositionFromAllContextsCommand cmd(node);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_FALSE(ctx_a->ContextContainsNode(node));
	EXPECT_FALSE(ctx_b->ContextContainsNode(node));

	cmd.undo_now();
	EXPECT_EQ(ctx_a->GetNodePositionInContext(node), QPointF(1.0, 1.0));
	EXPECT_EQ(ctx_b->GetNodePositionInContext(node), QPointF(2.0, 2.0));
}

TEST_F(NodeUndoTest, RenameCommandSetsAndRestoresLabels)
{
	auto *a = AddNode<olive::MathNode>();
	auto *b = AddNode<olive::SolidGenerator>();
	a->SetLabel(QStringLiteral("old_a"));
	b->SetLabel(QStringLiteral("old_b"));

	olive::NodeRenameCommand cmd(a, QStringLiteral("new_a"));
	cmd.AddNode(b, QStringLiteral("new_b"));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(a->GetLabel(), QStringLiteral("new_a"));
	EXPECT_EQ(b->GetLabel(), QStringLiteral("new_b"));

	cmd.undo_now();
	EXPECT_EQ(a->GetLabel(), QStringLiteral("old_a"));
	EXPECT_EQ(b->GetLabel(), QStringLiteral("old_b"));
}

TEST_F(NodeUndoTest, RenameCommandEmptyHasNoProject)
{
	olive::NodeRenameCommand cmd;
	EXPECT_EQ(cmd.GetRelevantProject(), nullptr);

	// redo/undo on an empty command must be harmless no-ops
	cmd.redo_now();
	cmd.undo_now();
}

TEST_F(NodeUndoTest, OverrideColorCommandSetsAndRestoresColor)
{
	auto *node = AddNode<olive::MathNode>();
	ASSERT_EQ(node->GetOverrideColor(), -1);

	olive::NodeOverrideColorCommand cmd(node, 5);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(node->GetOverrideColor(), 5);

	cmd.undo_now();
	EXPECT_EQ(node->GetOverrideColor(), -1);
}

TEST_F(NodeUndoTest, LinkCommandLinksAndUnlinks)
{
	auto *a = AddNode<olive::MathNode>();
	auto *b = AddNode<olive::SolidGenerator>();

	olive::NodeLinkCommand link_cmd(a, b, true);
	EXPECT_EQ(link_cmd.GetRelevantProject(), project_.get());

	link_cmd.redo_now();
	EXPECT_TRUE(olive::Node::AreLinked(a, b));

	link_cmd.undo_now();
	EXPECT_FALSE(olive::Node::AreLinked(a, b));

	olive::Node::Link(a, b);
	olive::NodeLinkCommand unlink_cmd(a, b, false);

	unlink_cmd.redo_now();
	EXPECT_FALSE(olive::Node::AreLinked(a, b));

	unlink_cmd.undo_now();
	EXPECT_TRUE(olive::Node::AreLinked(a, b));
}

TEST_F(NodeUndoTest, LinkCommandIgnoresAlreadyLinkedPair)
{
	auto *a = AddNode<olive::MathNode>();
	auto *b = AddNode<olive::SolidGenerator>();
	olive::Node::Link(a, b);

	olive::NodeLinkCommand cmd(a, b, true);
	cmd.redo_now();
	EXPECT_TRUE(olive::Node::AreLinked(a, b));

	// Undo must not unlink a pair that redo did not link
	cmd.undo_now();
	EXPECT_TRUE(olive::Node::AreLinked(a, b));
}

TEST_F(NodeUndoTest, UnlinkAllCommandRestoresAllLinks)
{
	auto *node = AddNode<olive::MathNode>();
	auto *a = AddNode<olive::SolidGenerator>();
	auto *b = AddNode<olive::SolidGenerator>();
	olive::Node::Link(node, a);
	olive::Node::Link(node, b);
	ASSERT_EQ(node->links().size(), 2);

	olive::NodeUnlinkAllCommand cmd(node);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_TRUE(node->links().isEmpty());
	EXPECT_FALSE(olive::Node::AreLinked(node, a));
	EXPECT_FALSE(olive::Node::AreLinked(node, b));

	cmd.undo_now();
	EXPECT_EQ(node->links().size(), 2);
	EXPECT_TRUE(olive::Node::AreLinked(node, a));
	EXPECT_TRUE(olive::Node::AreLinked(node, b));
}

TEST_F(NodeUndoTest, LinkManyCommandLinksAndUnlinksAllPairs)
{
	auto *a = AddNode<olive::MathNode>();
	auto *b = AddNode<olive::MathNode>();
	auto *c = AddNode<olive::SolidGenerator>();

	olive::NodeLinkManyCommand cmd({ a, b, c }, true);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_TRUE(olive::Node::AreLinked(a, b));
	EXPECT_TRUE(olive::Node::AreLinked(a, c));
	EXPECT_TRUE(olive::Node::AreLinked(b, c));

	cmd.undo_now();
	EXPECT_FALSE(olive::Node::AreLinked(a, b));
	EXPECT_FALSE(olive::Node::AreLinked(a, c));
	EXPECT_FALSE(olive::Node::AreLinked(b, c));
}

TEST_F(NodeUndoTest, ViewDeleteCommandRemovesNodeAndEdges)
{
	auto *context = AddNode<olive::Folder>();
	auto *a = AddNode<olive::SolidGenerator>();
	auto *b = AddNode<olive::MathNode>();

	olive::Node::ConnectEdge(
		a, olive::NodeInput(b, olive::MathNode::kParamAIn));
	context->SetNodePositionInContext(
		a, olive::Node::Position(QPointF(1.0, 2.0)));
	context->SetNodePositionInContext(
		b, olive::Node::Position(QPointF(3.0, 4.0)));

	olive::NodeViewDeleteCommand cmd;
	EXPECT_EQ(cmd.GetRelevantProject(), nullptr);

	cmd.AddNode(b, context);
	EXPECT_TRUE(cmd.ContainsNode(b, context));
	EXPECT_FALSE(cmd.ContainsNode(a, context));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();

	// b was only in this context and became fully disconnected, so it is
	// removed from the graph entirely
	EXPECT_FALSE(context->ContextContainsNode(b));
	EXPECT_EQ(b->project(), nullptr);
	EXPECT_TRUE(b->input_connections().empty());
	EXPECT_TRUE(a->output_connections().empty());

	cmd.undo_now();

	EXPECT_EQ(b->project(), project_.get());
	ASSERT_TRUE(context->ContextContainsNode(b));
	EXPECT_EQ(context->GetNodePositionInContext(b), QPointF(3.0, 4.0));
	ASSERT_EQ(a->output_connections().size(), 1);
	EXPECT_EQ(a->output_connections().front().second,
			  olive::NodeInput(b, olive::MathNode::kParamAIn));
}

TEST_F(NodeUndoTest, ViewDeleteCommandKeepsNodeConnectedOutsideContext)
{
	auto *context = AddNode<olive::Folder>();
	auto *a = AddNode<olive::SolidGenerator>();
	auto *outside = AddNode<olive::MathNode>();

	// "outside" is not in the context, so its edge keeps "a" in the graph
	olive::Node::ConnectEdge(
		a, olive::NodeInput(outside, olive::MathNode::kParamAIn));
	context->SetNodePositionInContext(
		a, olive::Node::Position(QPointF(7.0, 8.0)));

	olive::NodeViewDeleteCommand cmd;
	cmd.AddNode(a, context);

	cmd.redo_now();

	EXPECT_FALSE(context->ContextContainsNode(a));
	EXPECT_EQ(a->project(), project_.get());
	EXPECT_EQ(outside->input_connections().at(
				  olive::NodeInput(outside, olive::MathNode::kParamAIn)),
			  a);

	cmd.undo_now();

	ASSERT_TRUE(context->ContextContainsNode(a));
	EXPECT_EQ(context->GetNodePositionInContext(a), QPointF(7.0, 8.0));
}

TEST_F(NodeUndoTest, ParamSetKeyframingCommandTogglesKeyframing)
{
	auto *node = AddNode<olive::MathNode>();
	const olive::NodeInput input(node, olive::MathNode::kParamAIn);
	ASSERT_FALSE(input.IsKeyframing());

	olive::NodeParamSetKeyframingCommand cmd(input, true);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_TRUE(input.IsKeyframing());

	cmd.undo_now();
	EXPECT_FALSE(input.IsKeyframing());
}

TEST_F(NodeUndoTest, ParamInsertKeyframeCommandReparentsKeyframe)
{
	auto *node = AddNode<olive::MathNode>();

	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);

	olive::NodeParamInsertKeyframeCommand cmd(node, key);

	// The constructor takes ownership of the keyframe without inserting it
	EXPECT_NE(key->parent(), node);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(key->parent(), node);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.contains(key));

	cmd.undo_now();
	EXPECT_NE(key->parent(), node);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());
}

TEST_F(NodeUndoTest, ParamRemoveKeyframeCommandRestoresKeyframe)
{
	auto *node = AddNode<olive::MathNode>();

	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);
	key->setParent(node);
	ASSERT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.contains(key));

	olive::NodeParamRemoveKeyframeCommand cmd(key);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_NE(key->parent(), node);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());

	cmd.undo_now();
	EXPECT_EQ(key->parent(), node);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.contains(key));
}

TEST_F(NodeUndoTest, ParamSetKeyframeTimeCommandChangesTime)
{
	auto *node = AddNode<olive::MathNode>();
	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);
	key->setParent(node);

	olive::NodeParamSetKeyframeTimeCommand cmd(key, olive::rational(1, 2));
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(key->time(), olive::rational(1, 2));

	cmd.undo_now();
	EXPECT_EQ(key->time(), olive::rational(0));
}

TEST_F(NodeUndoTest, ParamSetKeyframeTimeCommandExplicitTimes)
{
	auto *node = AddNode<olive::MathNode>();
	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);
	key->setParent(node);

	olive::NodeParamSetKeyframeTimeCommand cmd(key, olive::rational(3, 4),
											   olive::rational(1, 4));

	cmd.redo_now();
	EXPECT_EQ(key->time(), olive::rational(3, 4));

	cmd.undo_now();
	EXPECT_EQ(key->time(), olive::rational(1, 4));
}

TEST_F(NodeUndoTest, ParamSetKeyframeValueCommandChangesValue)
{
	auto *node = AddNode<olive::MathNode>();
	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);
	key->setParent(node);

	olive::NodeParamSetKeyframeValueCommand cmd(key, 5.0);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_DOUBLE_EQ(key->value().toDouble(), 5.0);

	cmd.undo_now();
	EXPECT_DOUBLE_EQ(key->value().toDouble(), 1.0);
}

TEST_F(NodeUndoTest, ParamSetKeyframeValueCommandExplicitValues)
{
	auto *node = AddNode<olive::MathNode>();
	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);
	key->setParent(node);

	olive::NodeParamSetKeyframeValueCommand cmd(key, 7.5, 2.5);

	cmd.redo_now();
	EXPECT_DOUBLE_EQ(key->value().toDouble(), 7.5);

	cmd.undo_now();
	EXPECT_DOUBLE_EQ(key->value().toDouble(), 2.5);
}

TEST_F(NodeUndoTest, ParamSetStandardValueCommandSetsAndRestoresValue)
{
	auto *node = AddNode<olive::MathNode>();
	const olive::NodeKeyframeTrackReference ref(
		olive::NodeInput(node, olive::MathNode::kParamAIn), 0);
	ASSERT_DOUBLE_EQ(
		node->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 0.0);

	olive::NodeParamSetStandardValueCommand cmd(ref, 2.5);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_DOUBLE_EQ(
		node->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 2.5);

	cmd.undo_now();
	EXPECT_DOUBLE_EQ(
		node->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 0.0);
}

TEST_F(NodeUndoTest, ParamSetStandardValueCommandExplicitOldValue)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetStandardValue(olive::MathNode::kParamAIn, 10.0);
	const olive::NodeKeyframeTrackReference ref(
		olive::NodeInput(node, olive::MathNode::kParamAIn), 0);

	// Three-argument form with an explicit old value, as used by
	// SpeedDurationDialog
	olive::NodeParamSetStandardValueCommand cmd(ref, 20.0, 10.0);

	cmd.redo_now();
	EXPECT_DOUBLE_EQ(
		node->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 20.0);

	cmd.undo_now();
	EXPECT_DOUBLE_EQ(
		node->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 10.0);
}

TEST_F(NodeUndoTest, ParamSetSplitStandardValueCommandSetsAndRestoresSplit)
{
	auto *node = AddNode<olive::SolidGenerator>();
	const olive::NodeInput input(node, olive::SolidGenerator::kColorInput);

	const olive::SplitValue old_split = node->GetSplitStandardValue(input);
	ASSERT_EQ(old_split.size(), 4);

	const olive::SplitValue new_split = { 0.25, 0.5, 0.75, 1.0 };

	olive::NodeParamSetSplitStandardValueCommand cmd(input, new_split);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	const olive::SplitValue after = node->GetSplitStandardValue(input);
	ASSERT_EQ(after.size(), 4);
	EXPECT_DOUBLE_EQ(after.at(0).toDouble(), 0.25);
	EXPECT_DOUBLE_EQ(after.at(1).toDouble(), 0.5);
	EXPECT_DOUBLE_EQ(after.at(2).toDouble(), 0.75);
	EXPECT_DOUBLE_EQ(after.at(3).toDouble(), 1.0);

	cmd.undo_now();
	const olive::SplitValue restored = node->GetSplitStandardValue(input);
	ASSERT_EQ(restored.size(), 4);
	for (int i = 0; i < restored.size(); ++i) {
		EXPECT_DOUBLE_EQ(restored.at(i).toDouble(),
						 old_split.at(i).toDouble());
	}

	// The three-argument form takes the old value explicitly
	const olive::SplitValue explicit_new = { 1.0, 1.0, 1.0, 1.0 };
	olive::NodeParamSetSplitStandardValueCommand explicit_cmd(input,
															  explicit_new,
															  restored);
	explicit_cmd.redo_now();
	EXPECT_DOUBLE_EQ(node->GetSplitStandardValue(input).at(0).toDouble(), 1.0);

	explicit_cmd.undo_now();
	EXPECT_DOUBLE_EQ(node->GetSplitStandardValue(input).at(0).toDouble(),
					 restored.at(0).toDouble());
}

TEST_F(NodeUndoTest, ParamArrayAppendCommandAppendsAndRemoves)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	const int base = node->InputArraySize(olive::TextGeneratorV3::kArgsInput);

	olive::NodeParamArrayAppendCommand cmd(node,
										   olive::TextGeneratorV3::kArgsInput);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 1);

	cmd.undo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), base);
}

TEST_F(NodeUndoTest, ArrayInsertCommandInsertsAndRemovesElement)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	node->InputArrayAppend(olive::TextGeneratorV3::kArgsInput);
	const int base = node->InputArraySize(olive::TextGeneratorV3::kArgsInput);

	olive::NodeArrayInsertCommand cmd(node, olive::TextGeneratorV3::kArgsInput,
									  0);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 1);

	cmd.undo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), base);
}

TEST_F(NodeUndoTest, ArrayResizeCommandGrowAndShrink)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	const int base = node->InputArraySize(olive::TextGeneratorV3::kArgsInput);

	olive::NodeArrayResizeCommand cmd(node, olive::TextGeneratorV3::kArgsInput,
									  base + 3);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput),
			  base + 3);

	cmd.undo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), base);
}

TEST_F(NodeUndoTest, ArrayResizeCommandShrinkDisconnectsAndRestoresEdges)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	auto *output = AddNode<olive::MathNode>();

	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 3);
	const olive::NodeInput connected(node,
									 olive::TextGeneratorV3::kArgsInput, 2);
	olive::Node::ConnectEdge(output, connected);
	ASSERT_TRUE(connected.IsConnected());

	olive::NodeArrayResizeCommand cmd(node, olive::TextGeneratorV3::kArgsInput,
									  1);
	cmd.redo_now();

	// Shrinking removed elements 1 and 2; the edge into element 2 is dropped
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 1);
	EXPECT_FALSE(connected.IsConnected());
	EXPECT_TRUE(output->output_connections().empty());

	cmd.undo_now();

	// Undo restores both the array size and the removed connection
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 3);
	EXPECT_TRUE(connected.IsConnected());
	EXPECT_EQ(connected.GetConnectedOutput(), output);
}

TEST_F(NodeUndoTest, ArrayRemoveCommandPreservesKeyframesAndValues)
{
	auto *node = AddNode<olive::TextGeneratorV3>();
	node->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 2);

	const olive::NodeInput element(node, olive::TextGeneratorV3::kArgsInput,
								   1);
	node->SetStandardValue(element, QStringLiteral("hello"));
	node->SetInputIsKeyframing(element, true);

	auto *key = new olive::NodeKeyframe(
		olive::rational(0), QStringLiteral("key"), olive::NodeKeyframe::kLinear,
		0, 1, olive::TextGeneratorV3::kArgsInput);
	key->setParent(node);
	ASSERT_TRUE(node->GetKeyframeTracks(olive::TextGeneratorV3::kArgsInput, 1)
					.at(0)
					.contains(key));

	olive::NodeArrayRemoveCommand cmd(node, olive::TextGeneratorV3::kArgsInput,
									  1);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 1);
	EXPECT_NE(key->parent(), node);

	cmd.undo_now();
	EXPECT_EQ(node->InputArraySize(olive::TextGeneratorV3::kArgsInput), 2);
	EXPECT_EQ(key->parent(), node);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::TextGeneratorV3::kArgsInput, 1)
					.at(0)
					.contains(key));
	EXPECT_TRUE(node->IsInputKeyframing(olive::TextGeneratorV3::kArgsInput, 1));
	EXPECT_EQ(node->GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput, 1)
				  .at(0)
				  .toString(),
			  QStringLiteral("hello"));
}

TEST_F(NodeUndoTest, SetValueHintCommandSetsAndRestoresHint)
{
	auto *node = AddNode<olive::MathNode>();

	const olive::Node::ValueHint old_hint =
		node->GetValueHintForInput(olive::MathNode::kParamAIn, -1);

	const olive::Node::ValueHint new_hint({ olive::NodeValue::kVec2 }, 3,
										  QStringLiteral("tag"));
	olive::NodeSetValueHintCommand cmd(node, olive::MathNode::kParamAIn, -1,
									   new_hint);
	EXPECT_EQ(cmd.GetRelevantProject(), project_.get());

	cmd.redo_now();
	const olive::Node::ValueHint after =
		node->GetValueHintForInput(olive::MathNode::kParamAIn, -1);
	ASSERT_EQ(after.types().size(), 1);
	EXPECT_EQ(after.types().first(), olive::NodeValue::kVec2);
	EXPECT_EQ(after.index(), 3);
	EXPECT_EQ(after.tag(), QStringLiteral("tag"));

	cmd.undo_now();
	const olive::Node::ValueHint restored =
		node->GetValueHintForInput(olive::MathNode::kParamAIn, -1);
	EXPECT_EQ(restored.types().size(), old_hint.types().size());
	EXPECT_EQ(restored.index(), old_hint.index());
	EXPECT_EQ(restored.tag(), old_hint.tag());
}

TEST_F(NodeUndoTest, ImmediateRemoveAllKeyframesCommandRemovesKeys)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	auto *key_a = new olive::NodeKeyframe(olive::rational(0), 1.0,
										  olive::NodeKeyframe::kLinear, 0, -1,
										  olive::MathNode::kParamAIn);
	key_a->setParent(node);
	auto *key_b = new olive::NodeKeyframe(olive::rational(1), 2.0,
										  olive::NodeKeyframe::kLinear, 0, -1,
										  olive::MathNode::kParamAIn);
	key_b->setParent(node);

	olive::NodeInputImmediate *immediate =
		node->GetImmediate(olive::MathNode::kParamAIn, -1);
	ASSERT_NE(immediate, nullptr);
	ASSERT_EQ(immediate->keyframe_tracks().at(0).size(), 2);

	olive::NodeImmediateRemoveAllKeyframesCommand cmd(immediate);
	EXPECT_EQ(cmd.GetRelevantProject(), nullptr);

	cmd.redo_now();
	EXPECT_TRUE(immediate->keyframe_tracks().at(0).isEmpty());
	EXPECT_NE(key_a->parent(), node);
	EXPECT_NE(key_b->parent(), node);

	cmd.undo_now();
	// Undo restores the keyframes to the node and its tracks
	ASSERT_EQ(immediate->keyframe_tracks().at(0).size(), 2);
	EXPECT_EQ(immediate->keyframe_tracks().at(0).at(0), key_a);
	EXPECT_EQ(immediate->keyframe_tracks().at(0).at(1), key_b);
	EXPECT_EQ(key_a->parent(), node);
	EXPECT_EQ(key_b->parent(), node);
}
