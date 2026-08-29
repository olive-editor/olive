#include <gtest/gtest.h>

#include <memory>

#include <QSignalSpy>

#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/math/math/math.h"
#include "node/nodeundo.h"
#include "node/project.h"
#include "render/diskmanager.h"
#include "widget/curvewidget/curveview.h"
#include "widget/curvewidget/curvewidget.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/keyframeview/keyframeviewundo.h"
#include "widget/nodetreeview/nodetreeview.h"

using namespace olive;

namespace
{

// Keyframe deletion goes through the global undo stack hosted by Core
void EnsureAppSingletons()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
	if (!olive::DiskManager::instance()) {
		olive::DiskManager::CreateInstance();
	}
}

NodeKeyframe *InsertKeyframe(Node *node, const QString &input,
							 const rational &time, const QVariant &value,
							 int track = 0)
{
	auto *key =
		new NodeKeyframe(time, value, NodeKeyframe::kLinear, track, -1, input);
	NodeParamInsertKeyframeCommand(node, key).redo_now();
	return key;
}

} // namespace

class KeyframeViewTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();
		EnsureAppSingletons();

		project_ = std::make_unique<Project>();
		project_->Initialize();
	}

	MathNode *AddMathNode()
	{
		auto *node = new MathNode();
		node->setParent(project_.get());
		return node;
	}

	std::unique_ptr<Project> project_;
};

TEST_F(KeyframeViewTest, AddKeyframesOfNodeCreatesConnectionPerKeyframableInput)
{
	MathNode *node = AddMathNode();

	KeyframeView view;
	KeyframeView::NodeConnections map = view.AddKeyframesOfNode(node);

	// A float input has one element (the array-less -1) with one track
	ASSERT_TRUE(map.contains(MathNode::kParamAIn));
	const KeyframeView::InputConnections &param_a = map[MathNode::kParamAIn];
	ASSERT_EQ(param_a.size(), 1);
	ASSERT_EQ(param_a.first().size(), 1);
	EXPECT_NE(param_a.first().first(), nullptr);

	ASSERT_TRUE(map.contains(MathNode::kParamBIn));
	EXPECT_EQ(map[MathNode::kParamBIn].size(), 1);

	// The base-class enabled checkbox is keyframable too
	ASSERT_TRUE(map.contains(Node::kEnabledInput));
	EXPECT_EQ(map[Node::kEnabledInput].size(), 1);

	// The combo input is flagged not-keyframable, so it gets no connections
	ASSERT_TRUE(map.contains(MathNode::kMethodIn));
	EXPECT_TRUE(map[MathNode::kMethodIn].isEmpty());

	// One track connection each for enabled, param A and param B
	EXPECT_EQ(view.GetKeyframeTracks().size(), 3);
}

TEST_F(KeyframeViewTest, SelectAllAndDeselectAllUpdateSelection)
{
	MathNode *node = AddMathNode();
	NodeKeyframe *key_a = InsertKeyframe(node, MathNode::kParamAIn, rational(0), 0.0);
	NodeKeyframe *key_b = InsertKeyframe(node, MathNode::kParamAIn, rational(1), 1.0);

	KeyframeView view;
	view.AddKeyframesOfNode(node);

	QSignalSpy selection_spy(&view, &KeyframeView::SelectionChanged);

	EXPECT_TRUE(view.GetSelectedKeyframes().empty());

	view.SelectAll();
	EXPECT_EQ(view.GetSelectedKeyframes().size(), 2);
	EXPECT_GE(selection_spy.count(), 1);

	view.DeselectAll();
	EXPECT_TRUE(view.GetSelectedKeyframes().empty());
	EXPECT_GE(selection_spy.count(), 2);

	Q_UNUSED(key_a)
	Q_UNUSED(key_b)
}

TEST_F(KeyframeViewTest, RemoveKeyframesOfTrackDeselectsAndDetaches)
{
	MathNode *node = AddMathNode();
	InsertKeyframe(node, MathNode::kParamAIn, rational(0), 0.0);

	KeyframeView view;
	KeyframeViewInputConnection *connection = view.AddKeyframesOfTrack(
		NodeKeyframeTrackReference(NodeInput(node, MathNode::kParamAIn), 0));
	ASSERT_NE(connection, nullptr);
	ASSERT_EQ(view.GetKeyframeTracks().size(), 1);

	view.SelectAll();
	ASSERT_EQ(view.GetSelectedKeyframes().size(), 1);

	QSignalSpy selection_spy(&view, &KeyframeView::SelectionChanged);
	view.RemoveKeyframesOfTrack(connection);

	EXPECT_TRUE(view.GetKeyframeTracks().isEmpty());
	EXPECT_TRUE(view.GetSelectedKeyframes().empty());
	EXPECT_GE(selection_spy.count(), 1);

	// Removing again is a harmless no-op
	view.RemoveKeyframesOfTrack(connection);
	EXPECT_TRUE(view.GetKeyframeTracks().isEmpty());
}

TEST_F(KeyframeViewTest, ClearRemovesAllTracksAndSelection)
{
	MathNode *node = AddMathNode();
	InsertKeyframe(node, MathNode::kParamAIn, rational(0), 0.0);

	KeyframeView view;
	view.AddKeyframesOfNode(node);
	view.SelectAll();
	ASSERT_FALSE(view.GetKeyframeTracks().isEmpty());
	ASSERT_FALSE(view.GetSelectedKeyframes().empty());

	view.Clear();
	EXPECT_TRUE(view.GetKeyframeTracks().isEmpty());
	EXPECT_TRUE(view.GetSelectedKeyframes().empty());
}

TEST_F(KeyframeViewTest, DeleteSelectedPushesUndoableRemoval)
{
	MathNode *node = AddMathNode();
	NodeKeyframe *key = InsertKeyframe(node, MathNode::kParamAIn, rational(0), 0.0);

	KeyframeView view;
	view.AddKeyframesOfTrack(
		NodeKeyframeTrackReference(NodeInput(node, MathNode::kParamAIn), 0));
	view.SelectAll();
	view.DeleteSelected();

	// The command was executed on push: the keyframe is gone from the node
	EXPECT_TRUE(node->GetKeyframeTracks(MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());

	Core::instance()->undo_stack()->undo();
	EXPECT_TRUE(node->GetKeyframeTracks(MathNode::kParamAIn, -1)
					.at(0)
					.contains(key));

	Core::instance()->undo_stack()->redo();
	EXPECT_TRUE(node->GetKeyframeTracks(MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());

	// Keep the shared undo stack clean for other suites
	Core::instance()->undo_stack()->clear();
}

class KeyframeViewUndoTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<Project>();
		project_->Initialize();

		node_ = new MathNode();
		node_->setParent(project_.get());
	}

	std::unique_ptr<Project> project_;
	MathNode *node_ = nullptr;
};

TEST_F(KeyframeViewUndoTest, SetTypeCommandSwitchesAndRestoresType)
{
	NodeKeyframe *key =
		InsertKeyframe(node_, MathNode::kParamAIn, rational(0), 0.0);
	ASSERT_EQ(key->type(), NodeKeyframe::kLinear);

	KeyframeSetTypeCommand command(key, NodeKeyframe::kBezier);
	EXPECT_EQ(command.GetRelevantProject(), project_.get());

	command.redo_now();
	EXPECT_EQ(key->type(), NodeKeyframe::kBezier);

	command.undo_now();
	EXPECT_EQ(key->type(), NodeKeyframe::kLinear);
}

TEST_F(KeyframeViewUndoTest, SetBezierControlPointCapturesOldPointFromKeyframe)
{
	NodeKeyframe *key =
		InsertKeyframe(node_, MathNode::kParamAIn, rational(0), 0.0);
	key->set_type(NodeKeyframe::kBezier);
	key->set_bezier_control_in(QPointF(0.1, 0.2));

	KeyframeSetBezierControlPoint command(key, NodeKeyframe::kInHandle,
										  QPointF(0.5, 0.6));
	EXPECT_EQ(command.GetRelevantProject(), project_.get());

	command.redo_now();
	EXPECT_EQ(key->bezier_control_in(), QPointF(0.5, 0.6));

	command.undo_now();
	EXPECT_EQ(key->bezier_control_in(), QPointF(0.1, 0.2));
}

TEST_F(KeyframeViewUndoTest, SetBezierControlPointWithExplicitOldPoint)
{
	NodeKeyframe *key =
		InsertKeyframe(node_, MathNode::kParamAIn, rational(0), 0.0);
	key->set_type(NodeKeyframe::kBezier);

	// The four-argument overload does not read the current control point
	KeyframeSetBezierControlPoint command(key, NodeKeyframe::kOutHandle,
										  QPointF(0.7, 0.8), QPointF(0.3, 0.4));

	command.redo_now();
	EXPECT_EQ(key->bezier_control_out(), QPointF(0.7, 0.8));

	command.undo_now();
	EXPECT_EQ(key->bezier_control_out(), QPointF(0.3, 0.4));
}

class CurveViewTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();

		project_ = std::make_unique<Project>();
		project_->Initialize();

		solid_ = new SolidGenerator();
		solid_->setParent(project_.get());
	}

	NodeKeyframeTrackReference ColorTrackRef(int track) const
	{
		return NodeKeyframeTrackReference(
			NodeInput(solid_, SolidGenerator::kColorInput), track);
	}

	std::unique_ptr<Project> project_;
	SolidGenerator *solid_ = nullptr;
};

TEST_F(CurveViewTest, ConnectAndDisconnectInputManageTrackConnections)
{
	CurveView view;
	EXPECT_TRUE(view.GetConnections().isEmpty());

	view.ConnectInput(ColorTrackRef(0));
	EXPECT_EQ(view.GetConnections().size(), 1);
	EXPECT_TRUE(view.GetConnections().contains(ColorTrackRef(0)));
	EXPECT_EQ(view.GetConnections().value(ColorTrackRef(0))->GetReference(),
			  ColorTrackRef(0));

	// Connecting the same reference twice is a no-op
	view.ConnectInput(ColorTrackRef(0));
	EXPECT_EQ(view.GetConnections().size(), 1);

	// A color input has four tracks; connecting another track adds one more
	view.ConnectInput(ColorTrackRef(1));
	EXPECT_EQ(view.GetConnections().size(), 2);

	view.DisconnectInput(ColorTrackRef(0));
	EXPECT_EQ(view.GetConnections().size(), 1);
	EXPECT_FALSE(view.GetConnections().contains(ColorTrackRef(0)));

	// Disconnecting an unconnected reference is a no-op
	view.DisconnectInput(ColorTrackRef(0));
	EXPECT_EQ(view.GetConnections().size(), 1);
}

TEST_F(CurveViewTest, ConnectionReflectsLiveKeyframeList)
{
	CurveView view;
	view.ConnectInput(ColorTrackRef(0));

	KeyframeViewInputConnection *connection =
		view.GetConnections().value(ColorTrackRef(0));
	ASSERT_NE(connection, nullptr);
	EXPECT_TRUE(connection->GetKeyframes().isEmpty());

	NodeKeyframe *key =
		InsertKeyframe(solid_, SolidGenerator::kColorInput, rational(0), 0.5, 0);
	EXPECT_EQ(connection->GetKeyframes().size(), 1);
	EXPECT_EQ(connection->GetKeyframes().first(), key);
}

TEST_F(CurveViewTest, SetKeyframeTrackColorAppliesToBrush)
{
	CurveView view;

	// Setting the color before connecting is picked up on connect
	view.SetKeyframeTrackColor(ColorTrackRef(0), QColor(Qt::red));
	view.ConnectInput(ColorTrackRef(0));
	KeyframeViewInputConnection *connection =
		view.GetConnections().value(ColorTrackRef(0));
	ASSERT_NE(connection, nullptr);
	EXPECT_EQ(connection->GetBrush().color(), QColor(Qt::red));

	// Setting it afterwards updates the live connection
	view.SetKeyframeTrackColor(ColorTrackRef(0), QColor(Qt::blue));
	EXPECT_EQ(connection->GetBrush().color(), QColor(Qt::blue));
}

TEST_F(CurveViewTest, SelectKeyframesOfInputSelectsOnlyRequestedTrack)
{
	class SelectionProbeCurveView : public CurveView {
	public:
		using KeyframeView::IsKeyframeSelected;
	};

	SelectionProbeCurveView view;
	view.ConnectInput(ColorTrackRef(0));
	view.ConnectInput(ColorTrackRef(1));

	NodeKeyframe *key0 =
		InsertKeyframe(solid_, SolidGenerator::kColorInput, rational(0), 0.5, 0);
	NodeKeyframe *key1 =
		InsertKeyframe(solid_, SolidGenerator::kColorInput, rational(1), 0.6, 1);

	// Previously the reference was ignored and keyframes of every connected
	// track got selected.
	view.SelectKeyframesOfInput(ColorTrackRef(0));
	EXPECT_TRUE(view.IsKeyframeSelected(key0));
	EXPECT_FALSE(view.IsKeyframeSelected(key1));

	// Selecting the other track replaces the selection (DeselectAll first)
	view.SelectKeyframesOfInput(ColorTrackRef(1));
	EXPECT_FALSE(view.IsKeyframeSelected(key0));
	EXPECT_TRUE(view.IsKeyframeSelected(key1));
}

TEST(CurveWidget, VerticalScaleRoundTripsThroughView)
{
	ColorManager::SetUpDefaultConfig();

	CurveWidget widget;
	const double original = widget.GetVerticalScale();
	EXPECT_GT(original, 0.0);

	widget.SetVerticalScale(original * 2.0);
	EXPECT_DOUBLE_EQ(widget.GetVerticalScale(), original * 2.0);
}

TEST(CurveWidget, TreeSelectionConnectsTracksAndResolvesNodeId)
{
	ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();

	Project project;
	project.Initialize();
	auto *solid = new SolidGenerator();
	solid->setParent(&project);
	solid->Retranslate();

	CurveWidget widget;
	widget.SetNodes({ solid });

	auto *tree = widget.findChild<NodeTreeView *>();
	ASSERT_NE(tree, nullptr);
	ASSERT_EQ(tree->topLevelItemCount(), 1);

	// Nothing is connected until an input is selected in the tree
	EXPECT_EQ(widget.GetSelectedNodeWithID(solid->id()), nullptr);

	// The solid has two inputs ("enabled" from the base class, then "Color")
	QTreeWidgetItem *node_item = tree->topLevelItem(0);
	ASSERT_EQ(node_item->childCount(), 2);
	QTreeWidgetItem *color_item = node_item->child(1);

	color_item->setSelected(true);
	EXPECT_EQ(widget.GetSelectedNodeWithID(solid->id()), solid);
	EXPECT_EQ(widget.GetSelectedNodeWithID(QStringLiteral("org.example.bogus")),
			  nullptr);

	// Clearing the selection disconnects the tracks again
	tree->clearSelection();
	EXPECT_EQ(widget.GetSelectedNodeWithID(solid->id()), nullptr);
}
