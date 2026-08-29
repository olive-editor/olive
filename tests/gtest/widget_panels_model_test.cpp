#include <gtest/gtest.h>

#include <memory>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTreeWidgetItem>

#include "core.h"
#include "node/block/clip/clip.h"
#include "node/color/colormanager/colormanager.h"
#include "node/generator/solid/solid.h"
#include "node/input/multicam/multicamnode.h"
#include "node/math/math/math.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "render/diskmanager.h"
#include "render/rendermanager.h"
#include "task/task.h"
#include "undo/undostack.h"
#include "widget/colorbutton/colorbutton.h"
#include "widget/history/historywidget.h"
#include "widget/multicam/multicamwidget.h"
#include "widget/nodeparamview/nodeparamviewwidgetbridge.h"
#include "widget/nodetableview/nodetableview.h"
#include "widget/nodetreeview/nodetreeview.h"
#include "widget/nodeview/nodeviewscene.h"
#include "widget/slider/floatslider.h"
#include "widget/taskview/taskview.h"
#include "widget/taskview/taskviewitem.h"
#include "widget/timelinewidget/timelinewidgetselections.h"

using namespace olive;

namespace
{

// Bridges, history and multicam widgets all talk to the Core singleton
void EnsureAppSingletons()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
	if (!olive::DiskManager::instance()) {
		olive::DiskManager::CreateInstance();
	}
}

class DummyTask : public Task {
public:
	DummyTask()
	{
		SetTitle(QStringLiteral("Test Task"));
		SetError(QStringLiteral("boom"));
	}

protected:
	virtual bool Run() override
	{
		return true;
	}
};

class IncrementCommand : public UndoCommand {
public:
	explicit IncrementCommand(int *value)
		: value_(value)
	{
	}

	virtual Project *GetRelevantProject() const override
	{
		return nullptr;
	}

protected:
	virtual void redo() override
	{
		++(*value_);
	}

	virtual void undo() override
	{
		--(*value_);
	}

private:
	int *value_;
};

// NodeTreeView stores these on its items (mirrors the private constants)
const int kItemTypeRole = Qt::UserRole;
const int kItemInputReferenceRole = Qt::UserRole + 1;

} // namespace

class WidgetPanelsTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();
		EnsureAppSingletons();

		project_ = std::make_unique<Project>();
		project_->Initialize();
	}

	template <typename T> T *AddNode()
	{
		auto *node = new T();
		node->setParent(project_.get());
		return node;
	}

	std::unique_ptr<Project> project_;
};

TEST_F(WidgetPanelsTest, NodeTableSelectNodesCreatesTopLevelItems)
{
	auto *solid = AddNode<SolidGenerator>();

	NodeTableView view;
	EXPECT_EQ(view.topLevelItemCount(), 0);

	view.SelectNodes({ solid });
	ASSERT_EQ(view.topLevelItemCount(), 1);
	EXPECT_EQ(view.topLevelItem(0)->text(0), solid->GetLabelAndName());

	auto *math = AddNode<MathNode>();
	view.SelectNodes({ math });
	EXPECT_EQ(view.topLevelItemCount(), 2);

	view.DeselectNodes({ solid });
	EXPECT_EQ(view.topLevelItemCount(), 1);

	view.DeselectNodes({ math });
	EXPECT_EQ(view.topLevelItemCount(), 0);
}

TEST_F(WidgetPanelsTest, NodeTableSetTimePopulatesInputRows)
{
	auto *solid = AddNode<SolidGenerator>();
	solid->Retranslate();

	NodeTableView view;
	view.SelectNodes({ solid });

	QTreeWidgetItem *top = view.topLevelItem(0);
	ASSERT_NE(top, nullptr);

	// Rows appear for the base-class enabled input and the color input
	ASSERT_EQ(top->childCount(), 2);

	// The solid's color input appears as a named child row
	QTreeWidgetItem *color_row = nullptr;
	for (int i = 0; i < top->childCount(); i++) {
		if (top->child(i)->data(0, Qt::UserRole).toString() ==
			SolidGenerator::kColorInput) {
			color_row = top->child(i);
			break;
		}
	}
	ASSERT_NE(color_row, nullptr);
	EXPECT_EQ(color_row->text(0), solid->GetInputName(SolidGenerator::kColorInput));

	// The value row shows the data type name and the split RGBA columns
	ASSERT_EQ(color_row->childCount(), 1);
	QTreeWidgetItem *value_row = color_row->child(0);
	EXPECT_EQ(value_row->text(0),
			  NodeValue::GetPrettyDataTypeName(NodeValue::kColor));
	EXPECT_FALSE(value_row->text(1).isEmpty());
	for (int col = 2; col <= 5; col++) {
		EXPECT_FALSE(value_row->text(col).isEmpty()) << "column" << col;
	}

	// Re-evaluating at another time keeps the same structure
	view.SetTime(rational(1));
	EXPECT_EQ(top->childCount(), 2);
	EXPECT_EQ(color_row->childCount(), 1);
}

TEST_F(WidgetPanelsTest, NodeTreeSetNodesBuildsInputHierarchy)
{
	auto *math = AddNode<MathNode>();
	math->Retranslate();

	NodeTreeView view;
	view.SetNodes({ math });

	ASSERT_EQ(view.topLevelItemCount(), 1);
	QTreeWidgetItem *node_item = view.topLevelItem(0);
	EXPECT_EQ(node_item->data(0, kItemTypeRole).toInt(), 0); // kItemTypeNode

	// All four inputs are visible: the base-class enabled checkbox, the
	// method combo, and the two float params
	ASSERT_EQ(node_item->childCount(), 4);
	const QStringList expected_inputs = { Node::kEnabledInput, MathNode::kMethodIn,
										  MathNode::kParamAIn, MathNode::kParamBIn };
	for (int i = 0; i < expected_inputs.size(); i++) {
		QTreeWidgetItem *input_item = node_item->child(i);
		EXPECT_EQ(input_item->data(0, kItemTypeRole).toInt(), 1); // kItemTypeInput
		const NodeKeyframeTrackReference ref =
			input_item->data(0, kItemInputReferenceRole)
				.value<NodeKeyframeTrackReference>();
		EXPECT_EQ(ref.input().node(), math);
		EXPECT_EQ(ref.input().input(), expected_inputs.at(i));
	}
}

TEST_F(WidgetPanelsTest, NodeTreeOnlyShowKeyframableFiltersInputs)
{
	auto *math = AddNode<MathNode>();

	NodeTreeView view;
	view.SetOnlyShowKeyframable(true);
	view.SetNodes({ math });

	// The method combo is flagged not-keyframable; enabled and the two
	// float params remain
	ASSERT_EQ(view.topLevelItemCount(), 1);
	EXPECT_EQ(view.topLevelItem(0)->childCount(), 3);

	// Of a bare viewer's inputs only "enabled" is keyframable, so it is the
	// sole row left standing
	auto *viewer = AddNode<ViewerOutput>();
	view.SetNodes({ viewer });
	ASSERT_EQ(view.topLevelItemCount(), 1);
	EXPECT_EQ(view.topLevelItem(0)->childCount(), 1);

	// Without the filter its buffer inputs show up as well
	view.SetOnlyShowKeyframable(false);
	view.SetNodes({ viewer });
	ASSERT_EQ(view.topLevelItemCount(), 1);
	EXPECT_EQ(view.topLevelItem(0)->childCount(), 3);
}

TEST_F(WidgetPanelsTest, NodeTreeCheckboxesToggleEnableStateAndEmit)
{
	auto *math = AddNode<MathNode>();

	NodeTreeView view;
	view.SetCheckBoxesEnabled(true);
	view.SetNodes({ math });

	Node *node_signal_node = nullptr;
	bool node_signal_enabled = true;
	int node_emissions = 0;
	QObject::connect(&view, &NodeTreeView::NodeEnableChanged,
					 [&node_signal_node, &node_signal_enabled,
					  &node_emissions](Node *n, bool e) {
						 node_signal_node = n;
						 node_signal_enabled = e;
						 ++node_emissions;
					 });

	NodeKeyframeTrackReference input_signal_ref;
	bool input_signal_enabled = true;
	int input_emissions = 0;
	QObject::connect(&view, &NodeTreeView::InputEnableChanged,
					 [&input_signal_ref, &input_signal_enabled,
					  &input_emissions](const NodeKeyframeTrackReference &ref,
										bool e) {
						 input_signal_ref = ref;
						 input_signal_enabled = e;
						 ++input_emissions;
					 });

	QTreeWidgetItem *node_item = view.topLevelItem(0);
	ASSERT_NE(node_item, nullptr);
	ASSERT_EQ(node_item->checkState(0), Qt::Checked);

	// Unchecking the node disables it and emits
	node_item->setCheckState(0, Qt::Unchecked);
	EXPECT_EQ(node_emissions, 1);
	EXPECT_EQ(node_signal_node, math);
	EXPECT_FALSE(node_signal_enabled);
	EXPECT_FALSE(view.IsNodeEnabled(math));

	// Re-checking restores it
	node_item->setCheckState(0, Qt::Checked);
	EXPECT_EQ(node_emissions, 2);
	EXPECT_TRUE(node_signal_enabled);
	EXPECT_TRUE(view.IsNodeEnabled(math));

	// Same behavior on input rows
	QTreeWidgetItem *input_item = node_item->child(0);
	ASSERT_NE(input_item, nullptr);
	input_item->setCheckState(0, Qt::Unchecked);
	EXPECT_EQ(input_emissions, 1);
	EXPECT_EQ(input_signal_ref.input().input(), Node::kEnabledInput);
	EXPECT_FALSE(input_signal_enabled);
	EXPECT_FALSE(view.IsInputEnabled(input_signal_ref));
}

TEST_F(WidgetPanelsTest, NodeTreeKeyframeTracksBecomeRows)
{
	auto *solid = AddNode<SolidGenerator>();
	solid->Retranslate();

	NodeTreeView view;
	view.SetShowKeyframeTracksAsRows(true);
	view.SetNodes({ solid });

	QTreeWidgetItem *node_item = view.topLevelItem(0);
	ASSERT_NE(node_item, nullptr);
	ASSERT_EQ(node_item->childCount(), 2); // enabled + color

	// The four-track color input expands into one row per track
	QTreeWidgetItem *color_item = node_item->child(1);
	EXPECT_EQ(color_item->text(0), QStringLiteral("Color"));
	ASSERT_EQ(color_item->childCount(), 4);
	const QStringList track_names = { QStringLiteral("R"), QStringLiteral("G"),
									  QStringLiteral("B"), QStringLiteral("A") };
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(color_item->child(i)->text(0), track_names.at(i));
		const NodeKeyframeTrackReference ref =
			color_item->child(i)
				->data(0, kItemInputReferenceRole)
				.value<NodeKeyframeTrackReference>();
		EXPECT_EQ(ref.track(), i);
	}

	// A single-track float input stays a single row
	auto *math = AddNode<MathNode>();
	math->Retranslate();
	view.SetNodes({ math });
	QTreeWidgetItem *param_item =
		view.topLevelItem(0)->child(2); // after enabled and the method combo
	ASSERT_NE(param_item, nullptr);
	EXPECT_EQ(param_item->text(0), QStringLiteral("Value"));
	EXPECT_EQ(param_item->childCount(), 0);
}

TEST_F(WidgetPanelsTest, BridgeCreatesSliderForFloatInput)
{
	auto *math = AddNode<MathNode>();

	QWidget parent;
	NodeParamViewWidgetBridge bridge(NodeInput(math, MathNode::kParamAIn), &parent);

	ASSERT_EQ(bridge.widgets().size(), 1);
	EXPECT_NE(qobject_cast<FloatSlider *>(bridge.widgets().first()), nullptr);
}

TEST_F(WidgetPanelsTest, BridgeCreatesColorButtonForColorInput)
{
	auto *solid = AddNode<SolidGenerator>();

	QWidget parent;
	NodeParamViewWidgetBridge bridge(NodeInput(solid, SolidGenerator::kColorInput),
									 &parent);

	ASSERT_EQ(bridge.widgets().size(), 1);
	EXPECT_NE(qobject_cast<ColorButton *>(bridge.widgets().first()), nullptr);
}

TEST_F(WidgetPanelsTest, BridgeCreatesComboBoxForComboInput)
{
	auto *math = AddNode<MathNode>();
	math->Retranslate();

	QWidget parent;
	NodeParamViewWidgetBridge bridge(NodeInput(math, MathNode::kMethodIn), &parent);

	ASSERT_EQ(bridge.widgets().size(), 1);
	auto *combo = qobject_cast<QComboBox *>(bridge.widgets().first());
	ASSERT_NE(combo, nullptr);
	EXPECT_EQ(combo->count(), math->GetComboBoxStrings(MathNode::kMethodIn).size());
	EXPECT_GT(combo->count(), 0);
}

TEST_F(WidgetPanelsTest, BridgeCreatesCheckBoxForBooleanInput)
{
	auto *clip = AddNode<ClipBlock>();

	QWidget parent;
	NodeParamViewWidgetBridge bridge(NodeInput(clip, ClipBlock::kReverseInput),
									 &parent);

	ASSERT_EQ(bridge.widgets().size(), 1);
	EXPECT_NE(qobject_cast<QCheckBox *>(bridge.widgets().first()), nullptr);
}

TEST_F(WidgetPanelsTest, BridgeUpdatesWidgetWhenNodeValueChanges)
{
	auto *math = AddNode<MathNode>();
	auto *viewer = AddNode<ViewerOutput>();

	QWidget parent;
	NodeParamViewWidgetBridge bridge(NodeInput(math, MathNode::kParamAIn), &parent);
	auto *slider = qobject_cast<FloatSlider *>(bridge.widgets().first());
	ASSERT_NE(slider, nullptr);
	EXPECT_DOUBLE_EQ(slider->GetValue(), 0.0);

	// The bridge only refreshes widgets for value changes at the playhead
	// of a connected time target
	bridge.SetTimeTarget(viewer);

	math->SetStandardValue(MathNode::kParamAIn, 2.5);
	EXPECT_DOUBLE_EQ(slider->GetValue(), 2.5);
}

TEST_F(WidgetPanelsTest, BridgePushesUndoCommandWhenWidgetChanges)
{
	auto *math = AddNode<MathNode>();
	math->Retranslate();
	ASSERT_EQ(math->GetStandardValue(MathNode::kMethodIn).toInt(), 0);

	QWidget parent;
	NodeParamViewWidgetBridge bridge(NodeInput(math, MathNode::kMethodIn), &parent);
	auto *combo = qobject_cast<QComboBox *>(bridge.widgets().first());
	ASSERT_NE(combo, nullptr);
	ASSERT_GT(combo->count(), 1);

	combo->setCurrentIndex(1);
	EXPECT_EQ(math->GetStandardValue(MathNode::kMethodIn).toInt(), 1);

	Core::instance()->undo_stack()->undo();
	EXPECT_EQ(math->GetStandardValue(MathNode::kMethodIn).toInt(), 0);
	Core::instance()->undo_stack()->clear();
}

TEST(TaskView, TaskLifecycleUpdatesItems)
{
	TaskView view(nullptr);
	DummyTask task;

	view.AddTask(&task);

	auto *item = view.findChild<TaskViewItem *>();
	ASSERT_NE(item, nullptr);

	// Title label mirrors the task title
	bool found_title = false;
	foreach (QLabel *label, item->findChildren<QLabel *>()) {
		if (label->text() == QStringLiteral("Test Task")) {
			found_title = true;
			break;
		}
	}
	EXPECT_TRUE(found_title);

	// Progress signals drive the progress bar
	auto *bar = item->findChild<QProgressBar *>();
	ASSERT_NE(bar, nullptr);
	emit task.ProgressChanged(0.5);
	EXPECT_EQ(bar->value(), 50);

	// The cancel button relays the task through TaskCancelled
	Task *cancelled = nullptr;
	QObject::connect(&view, &TaskView::TaskCancelled,
					 [&cancelled](Task *t) { cancelled = t; });
	auto *cancel_button = item->findChild<QPushButton *>();
	ASSERT_NE(cancel_button, nullptr);
	cancel_button->click();
	EXPECT_EQ(cancelled, &task);

	// Failure swaps in the error label
	view.TaskFailed(&task);
	bool found_error = false;
	foreach (QLabel *label, item->findChildren<QLabel *>()) {
		if (label->text().contains(QStringLiteral("boom"))) {
			found_error = true;
			break;
		}
	}
	EXPECT_TRUE(found_error);

	// Removal deletes the item once deferred deletions are processed
	view.RemoveTask(&task);
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	EXPECT_EQ(view.findChild<TaskViewItem *>(), nullptr);
}

TEST(HistoryWidget, ReflectsAndDrivesUndoStack)
{
	EnsureAppSingletons();
	UndoStack *stack = Core::instance()->undo_stack();
	stack->clear();

	HistoryWidget widget;
	EXPECT_EQ(widget.model(), stack);

	int counter = 0;
	stack->push(new IncrementCommand(&counter), QStringLiteral("First"));
	stack->push(new IncrementCommand(&counter), QStringLiteral("Second"));
	EXPECT_EQ(counter, 2);
	EXPECT_EQ(stack->rowCount(), 3);

	// Row 0 is the "New/Open Project" sentinel the stack starts with, so
	// "First" lives at row 1 and "Second" at row 2. Moving the current row
	// jumps the stack to row+1 applied commands (this is how clicking a
	// history row works).
	widget.selectionModel()->setCurrentIndex(stack->index(1, 0),
											 QItemSelectionModel::ClearAndSelect |
												 QItemSelectionModel::Rows);
	EXPECT_EQ(counter, 1);
	EXPECT_TRUE(stack->CanRedo());

	// Moving to the second entry redoes everything again
	widget.selectionModel()->setCurrentIndex(stack->index(2, 0),
											 QItemSelectionModel::ClearAndSelect |
												 QItemSelectionModel::Rows);
	EXPECT_EQ(counter, 2);
	EXPECT_FALSE(stack->CanRedo());

	// Moving back to the sentinel row undoes both commands
	widget.selectionModel()->setCurrentIndex(stack->index(0, 0),
											 QItemSelectionModel::ClearAndSelect |
												 QItemSelectionModel::Rows);
	EXPECT_EQ(counter, 0);
	EXPECT_TRUE(stack->CanRedo());

	stack->clear();
}

TEST(TimelineSelections, ShiftTimeMovesAllRanges)
{
	TimelineWidgetSelections sel;
	const Track::Reference video0(Track::kVideo, 0);
	sel.insert(video0,
			   TimeRangeList({ TimeRange(rational(0), rational(10)) }));

	sel.ShiftTime(rational(5));
	const TimeRangeList list = sel.value(video0);
	ASSERT_EQ(list.size(), 1);
	EXPECT_EQ(list.first().in(), rational(5));
	EXPECT_EQ(list.first().out(), rational(15));
}

TEST(TimelineSelections, ShiftTracksReindexesMatchingTypeOnly)
{
	TimelineWidgetSelections sel;
	sel.insert(Track::Reference(Track::kVideo, 0),
			   TimeRangeList({ TimeRange(rational(0), rational(10)) }));
	sel.insert(Track::Reference(Track::kVideo, 1),
			   TimeRangeList({ TimeRange(rational(0), rational(10)) }));
	sel.insert(Track::Reference(Track::kAudio, 0),
			   TimeRangeList({ TimeRange(rational(0), rational(10)) }));

	sel.ShiftTracks(Track::kVideo, 2);

	EXPECT_FALSE(sel.contains(Track::Reference(Track::kVideo, 0)));
	EXPECT_FALSE(sel.contains(Track::Reference(Track::kVideo, 1)));
	EXPECT_TRUE(sel.contains(Track::Reference(Track::kVideo, 2)));
	EXPECT_TRUE(sel.contains(Track::Reference(Track::kVideo, 3)));
	EXPECT_TRUE(sel.contains(Track::Reference(Track::kAudio, 0)));
}

TEST(TimelineSelections, TrimInAndOutAdjustRangeEnds)
{
	TimelineWidgetSelections in_sel;
	const Track::Reference video0(Track::kVideo, 0);
	in_sel.insert(video0,
				  TimeRangeList({ TimeRange(rational(0), rational(10)) }));
	in_sel.TrimIn(rational(2));
	EXPECT_EQ(in_sel.value(video0).first().in(), rational(2));
	EXPECT_EQ(in_sel.value(video0).first().out(), rational(10));

	TimelineWidgetSelections out_sel;
	out_sel.insert(video0,
				   TimeRangeList({ TimeRange(rational(0), rational(10)) }));
	out_sel.TrimOut(rational(-3));
	EXPECT_EQ(out_sel.value(video0).first().in(), rational(0));
	EXPECT_EQ(out_sel.value(video0).first().out(), rational(7));
}

TEST(TimelineSelections, SubtractSplitsAndIgnoresForeignTracks)
{
	TimelineWidgetSelections ours;
	const Track::Reference video0(Track::kVideo, 0);
	ours.insert(video0,
				TimeRangeList({ TimeRange(rational(0), rational(10)) }));

	TimelineWidgetSelections theirs;
	theirs.insert(video0,
				  TimeRangeList({ TimeRange(rational(3), rational(5)) }));
	theirs.insert(Track::Reference(Track::kAudio, 0),
				  TimeRangeList({ TimeRange(rational(0), rational(99)) }));

	TimelineWidgetSelections result = ours.Subtracted(theirs);

	// The original is untouched by the const version
	EXPECT_EQ(ours.value(video0).size(), 1);

	const TimeRangeList remaining = result.value(video0);
	ASSERT_EQ(remaining.size(), 2);
	EXPECT_EQ(remaining.at(0), TimeRange(rational(0), rational(3)));
	EXPECT_EQ(remaining.at(1), TimeRange(rational(5), rational(10)));

	// In-place Subtract drops the subtracted span as well
	ours.Subtract(theirs);
	EXPECT_EQ(ours.value(video0).size(), 2);
}

TEST(NodeViewScene, AddAndRemoveContexts)
{
	ColorManager::SetUpDefaultConfig();
	Project project;
	project.Initialize();
	auto *folder = new Folder();
	folder->setParent(&project);

	NodeViewScene scene;
	EXPECT_TRUE(scene.context_map().isEmpty());

	NodeViewContext *ctx = scene.AddContext(folder);
	ASSERT_NE(ctx, nullptr);
	EXPECT_TRUE(scene.context_map().contains(folder));
	EXPECT_EQ(ctx->GetContext(), folder);
	EXPECT_TRUE(scene.items().contains(ctx));

	// Re-adding the same node returns the existing context item
	EXPECT_EQ(scene.AddContext(folder), ctx);
	EXPECT_EQ(scene.context_map().size(), 1);

	scene.RemoveContext(folder);
	EXPECT_TRUE(scene.context_map().isEmpty());
}

TEST(NodeViewScene, FlowDirectionControlsOrientation)
{
	NodeViewScene scene;
	EXPECT_EQ(scene.GetFlowDirection(), NodeViewCommon::kLeftToRight);
	EXPECT_EQ(scene.GetFlowOrientation(), Qt::Horizontal);

	scene.SetFlowDirection(NodeViewCommon::kTopToBottom);
	EXPECT_EQ(scene.GetFlowDirection(), NodeViewCommon::kTopToBottom);
	EXPECT_EQ(scene.GetFlowOrientation(), Qt::Vertical);
}

class MulticamWidgetTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		ColorManager::SetUpDefaultConfig();
		EnsureAppSingletons();

		// The display widget pulls the render backend off RenderManager,
		// which the bare Core singleton does not create (Core::Start()
		// would); viewer_display_repro_test does the same
		created_render_manager_ = (RenderManager::instance() == nullptr);
		if (created_render_manager_) {
			RenderManager::CreateInstance();
		}

		project_ = std::make_unique<Project>();
		project_->Initialize();
	}

	void TearDown() override
	{
		project_.reset();

		// Leave the singleton the way we found it: render suites check
		// RenderManager::instance() for null in their own teardowns
		if (created_render_manager_) {
			RenderManager::DestroyInstance();
			created_render_manager_ = false;
		}
	}

	std::unique_ptr<Project> project_;
	bool created_render_manager_ = false;
};

TEST_F(MulticamWidgetTest, ConstructionCreatesDisplay)
{
	MulticamWidget widget;
	EXPECT_NE(widget.GetDisplayWidget(), nullptr);
	EXPECT_EQ(widget.GetConnectedNode(), nullptr);
}

TEST_F(MulticamWidgetTest, SwitchWithoutTimestampAppliesImmediately)
{
	auto *viewer = new ViewerOutput();
	viewer->setParent(project_.get());
	auto *node = new MultiCamNode();
	node->setParent(project_.get());
	auto *clip = new ClipBlock();
	clip->setParent(project_.get());

	MulticamWidget widget;
	widget.SetMulticamNode(viewer, node, clip, rational());
	EXPECT_EQ(widget.GetConnectedNode(), viewer);
}

TEST_F(MulticamWidgetTest, FutureSwitchWaitsForPlayheadToAdvance)
{
	auto *viewer_a = new ViewerOutput();
	viewer_a->setParent(project_.get());
	auto *viewer_b = new ViewerOutput();
	viewer_b->setParent(project_.get());
	auto *node = new MultiCamNode();
	node->setParent(project_.get());
	auto *clip = new ClipBlock();
	clip->setParent(project_.get());

	MulticamWidget widget;
	widget.SetMulticamNode(viewer_a, node, clip, rational());
	ASSERT_EQ(widget.GetConnectedNode(), viewer_a);

	// A switch stamped for a later time is queued, not applied
	widget.SetMulticamNode(viewer_b, node, clip, rational(5));
	EXPECT_EQ(widget.GetConnectedNode(), viewer_a);

	// Once playback time advances, the queued switch takes effect
	viewer_a->SetPlayhead(rational(1));
	EXPECT_EQ(widget.GetConnectedNode(), viewer_b);
}
