#include <gtest/gtest.h>

#include <memory>

#include <QString>
#include <QVariant>

#include "node/block/gap/gap.h"
#include "node/color/colormanager/colormanager.h"
#include "node/globals.h"
#include "node/input/time/timeinput.h"
#include "node/keyframe.h"
#include "node/project.h"
#include "node/time/timeformat/timeformat.h"
#include "node/time/timeoffset/timeoffsetnode.h"
#include "node/time/timeremap/timeremap.h"
#include "node/traverser.h"
#include "olive/core/util/rational.h"
#include "olive/core/util/timerange.h"

namespace
{

class NodeTimeTest : public ::testing::Test {
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

	olive::NodeKeyframe *AddKey(olive::Node *node, const QString &input,
								const olive::core::rational &time,
								const QVariant &value)
	{
		auto *key = new olive::NodeKeyframe(
			time, value, olive::NodeKeyframe::kLinear, 0, -1, input);
		key->setParent(node);
		return key;
	}

	// Generates the node's output table at a single time with a fresh
	// traverser (the traverser caches tables per node+range, so reusing one
	// would return stale values after the node's parameters change)
	olive::NodeValueTable GenerateTable(const olive::Node *node,
										const olive::core::rational &time)
	{
		olive::NodeTraverser traverser;
		return traverser.GenerateTable(
			node, olive::TimeRange(time, time + olive::core::rational(1, 30)));
	}

	std::unique_ptr<olive::Project> project_;
};

} // namespace

TEST(GapBlock, Metadata)
{
	olive::GapBlock gap;

	EXPECT_EQ(gap.Name(), QStringLiteral("Gap"));
	EXPECT_EQ(gap.id(), QStringLiteral("org.olivevideoeditor.Olive.gap"));
	EXPECT_FALSE(gap.Description().isEmpty());
	EXPECT_TRUE(gap.Category().contains(olive::Node::kCategoryTimeline));
}

TEST(GapBlock, DefaultLengthIsZero)
{
	olive::GapBlock gap;

	EXPECT_EQ(gap.length(), olive::core::rational(0));
}

TEST_F(NodeTimeTest, GapBlockStoresRationalLength)
{
	auto *gap = AddNode<olive::GapBlock>();

	gap->set_length_and_media_out(olive::core::rational(7, 2));
	EXPECT_EQ(gap->length(), olive::core::rational(7, 2));

	gap->set_length_and_media_out(olive::core::rational(0));
	EXPECT_EQ(gap->length(), olive::core::rational(0));
}

TEST(TimeOffsetNode, Metadata)
{
	olive::TimeOffsetNode offset;

	EXPECT_EQ(offset.Name(), QStringLiteral("Time Offset"));
	EXPECT_EQ(offset.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.timeoffset"));
	EXPECT_FALSE(offset.Description().isEmpty());
	EXPECT_TRUE(offset.Category().contains(olive::Node::kCategoryTime));
}

TEST(TimeOffsetNode, InputFlags)
{
	olive::TimeOffsetNode offset;

	// The time parameter is keyframable but cannot take an edge
	EXPECT_FALSE(offset.IsInputConnectable(olive::TimeOffsetNode::kTimeInput));
	EXPECT_TRUE(offset.IsInputKeyframable(olive::TimeOffsetNode::kTimeInput));

	// The data input takes an edge but cannot be keyframed
	EXPECT_TRUE(offset.IsInputConnectable(olive::TimeOffsetNode::kInputInput));
	EXPECT_FALSE(offset.IsInputKeyframable(olive::TimeOffsetNode::kInputInput));

	// The time offset defaults to zero
	EXPECT_EQ(offset.GetStandardValue(olive::TimeOffsetNode::kTimeInput)
				  .value<olive::core::rational>(),
			  olive::core::rational(0));
}

TEST(TimeOffsetNode, RetranslateSetsInputNames)
{
	olive::TimeOffsetNode offset;

	offset.Retranslate();

	EXPECT_EQ(offset.GetInputName(olive::TimeOffsetNode::kTimeInput),
			  QStringLiteral("Time"));
	EXPECT_EQ(offset.GetInputName(olive::TimeOffsetNode::kInputInput),
			  QStringLiteral("Input"));
}

TEST_F(NodeTimeTest, TimeOffsetAppliesStaticOffset)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetStandardValue(olive::TimeOffsetNode::kTimeInput,
							 QVariant::fromValue(olive::core::rational(3)));

	// The connected input is evaluated offset seconds later
	EXPECT_EQ(offset->InputTimeAdjustment(
				  olive::TimeOffsetNode::kInputInput, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(5),
							   olive::core::rational(7)));

	// Any other input passes time through unchanged
	const olive::TimeRange range(olive::core::rational(2),
								 olive::core::rational(4));
	EXPECT_EQ(offset->InputTimeAdjustment(olive::TimeOffsetNode::kTimeInput,
										  -1, range, true),
			  range);
}

TEST_F(NodeTimeTest, TimeOffsetAppliesNegativeOffset)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetStandardValue(olive::TimeOffsetNode::kTimeInput,
							 QVariant::fromValue(olive::core::rational(-3)));

	// Negative offsets move the requested time before the sequence time,
	// even across zero
	EXPECT_EQ(offset->InputTimeAdjustment(
				  olive::TimeOffsetNode::kInputInput, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(-1),
							   olive::core::rational(1)));
}

TEST_F(NodeTimeTest, TimeOffsetZeroOffsetIsIdentity)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();

	const olive::TimeRange range(olive::core::rational(2),
								 olive::core::rational(4));
	EXPECT_EQ(offset->InputTimeAdjustment(olive::TimeOffsetNode::kInputInput,
										  -1, range, true),
			  range);
}

TEST_F(NodeTimeTest, TimeOffsetOutputAdjustmentAppliesInverseOffset)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetStandardValue(olive::TimeOffsetNode::kTimeInput,
							 QVariant::fromValue(olive::core::rational(3)));

	// The inverse mapping subtracts the offset again: input-side times are
	// mapped back to the output by the negated offset
	EXPECT_EQ(offset->OutputTimeAdjustment(
				  olive::TimeOffsetNode::kInputInput, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4))),
			  olive::TimeRange(olive::core::rational(-1),
							   olive::core::rational(1)));

	// Non-input inputs never adjust time
	const olive::TimeRange range(olive::core::rational(2),
								 olive::core::rational(4));
	EXPECT_EQ(offset->OutputTimeAdjustment(olive::TimeOffsetNode::kTimeInput,
										   -1, range),
			  range);

	// Round trip through both adjustments returns the original range
	EXPECT_EQ(offset->OutputTimeAdjustment(
				  olive::TimeOffsetNode::kInputInput, -1,
				  offset->InputTimeAdjustment(
					  olive::TimeOffsetNode::kInputInput, -1, range, true)),
			  range);
}

TEST_F(NodeTimeTest, TimeOffsetAppliesKeyframedOffset)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetInputIsKeyframing(olive::TimeOffsetNode::kTimeInput, true);

	AddKey(offset, olive::TimeOffsetNode::kTimeInput, olive::core::rational(0),
		   QVariant::fromValue(olive::core::rational(0)));
	AddKey(offset, olive::TimeOffsetNode::kTimeInput,
		   olive::core::rational(10),
		   QVariant::fromValue(olive::core::rational(10)));

	// The offset is sampled per endpoint: 2 + 2 = 4 and 4 + 4 = 8
	EXPECT_EQ(offset->InputTimeAdjustment(
				  olive::TimeOffsetNode::kInputInput, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(4),
							   olive::core::rational(8)));
}

TEST_F(NodeTimeTest, TimeOffsetValuePassesConnectedInputThrough)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetStandardValue(olive::TimeOffsetNode::kTimeInput,
							 QVariant::fromValue(olive::core::rational(5)));

	auto *time = AddNode<olive::TimeInput>();
	olive::Node::ConnectEdge(
		time, olive::NodeInput(offset, olive::TimeOffsetNode::kInputInput));

	// The connected node is evaluated at the offset time: 3 + 5 = 8
	const olive::NodeValueTable table = GenerateTable(offset, olive::core::rational(3));
	EXPECT_DOUBLE_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 8.0);
}

TEST_F(NodeTimeTest, TimeOffsetValueWithoutConnectionProducesNoFloat)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();

	// With nothing connected the node pushes the (typeless) standard value
	const olive::NodeValueTable table = GenerateTable(offset, olive::core::rational(3));
	EXPECT_EQ(table.Get(olive::NodeValue::kFloat).type(),
			  olive::NodeValue::kNone);
}

TEST(TimeRemapNode, Metadata)
{
	olive::TimeRemapNode remap;

	EXPECT_EQ(remap.Name(), QStringLiteral("Time Remap"));
	EXPECT_EQ(remap.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.timeremap"));
	EXPECT_FALSE(remap.Description().isEmpty());
	EXPECT_TRUE(remap.Category().contains(olive::Node::kCategoryTime));
}

TEST(TimeRemapNode, InputFlags)
{
	olive::TimeRemapNode remap;

	EXPECT_FALSE(remap.IsInputConnectable(olive::TimeRemapNode::kTimeInput));
	EXPECT_TRUE(remap.IsInputKeyframable(olive::TimeRemapNode::kTimeInput));

	EXPECT_TRUE(remap.IsInputConnectable(olive::TimeRemapNode::kInputInput));
	EXPECT_FALSE(remap.IsInputKeyframable(olive::TimeRemapNode::kInputInput));

	EXPECT_EQ(remap.GetStandardValue(olive::TimeRemapNode::kTimeInput)
				  .value<olive::core::rational>(),
			  olive::core::rational(0));
}

TEST(TimeRemapNode, RetranslateSetsInputNames)
{
	olive::TimeRemapNode remap;

	remap.Retranslate();

	EXPECT_EQ(remap.GetInputName(olive::TimeRemapNode::kTimeInput),
			  QStringLiteral("Time"));
	EXPECT_EQ(remap.GetInputName(olive::TimeRemapNode::kInputInput),
			  QStringLiteral("Input"));
}

TEST_F(NodeTimeTest, TimeRemapStaticTimeCollapsesRange)
{
	auto *remap = AddNode<olive::TimeRemapNode>();
	remap->SetStandardValue(olive::TimeRemapNode::kTimeInput,
							QVariant::fromValue(olive::core::rational(7)));

	// A constant remap maps every sequence time onto the same media time
	const olive::TimeRange adjusted = remap->InputTimeAdjustment(
		olive::TimeRemapNode::kInputInput, -1,
		olive::TimeRange(olive::core::rational(2), olive::core::rational(4)),
		true);
	EXPECT_EQ(adjusted.in(), olive::core::rational(7));
	EXPECT_EQ(adjusted.out(), olive::core::rational(7));
}

TEST_F(NodeTimeTest, TimeRemapNonInputPassesThrough)
{
	auto *remap = AddNode<olive::TimeRemapNode>();
	remap->SetStandardValue(olive::TimeRemapNode::kTimeInput,
							QVariant::fromValue(olive::core::rational(7)));

	const olive::TimeRange range(olive::core::rational(2),
								 olive::core::rational(4));
	EXPECT_EQ(remap->InputTimeAdjustment(olive::TimeRemapNode::kTimeInput, -1,
										 range, true),
			  range);
	EXPECT_EQ(remap->OutputTimeAdjustment(olive::TimeRemapNode::kInputInput,
										  -1, range),
			  range);
	EXPECT_EQ(remap->OutputTimeAdjustment(olive::TimeRemapNode::kTimeInput, -1,
										  range),
			  range);
}

TEST_F(NodeTimeTest, TimeRemapAppliesKeyframedLinearRamp)
{
	auto *remap = AddNode<olive::TimeRemapNode>();
	remap->SetInputIsKeyframing(olive::TimeRemapNode::kTimeInput, true);

	AddKey(remap, olive::TimeRemapNode::kTimeInput, olive::core::rational(0),
		   QVariant::fromValue(olive::core::rational(0)));
	AddKey(remap, olive::TimeRemapNode::kTimeInput, olive::core::rational(10),
		   QVariant::fromValue(olive::core::rational(100)));

	// The linear ramp multiplies time by ten: 2 -> 20 and 4 -> 40
	EXPECT_EQ(remap->InputTimeAdjustment(
				  olive::TimeRemapNode::kInputInput, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(20),
							   olive::core::rational(40)));
}

TEST_F(NodeTimeTest, TimeRemapReversedRampNormalizesRange)
{
	auto *remap = AddNode<olive::TimeRemapNode>();
	remap->SetInputIsKeyframing(olive::TimeRemapNode::kTimeInput, true);

	AddKey(remap, olive::TimeRemapNode::kTimeInput, olive::core::rational(0),
		   QVariant::fromValue(olive::core::rational(10)));
	AddKey(remap, olive::TimeRemapNode::kTimeInput, olive::core::rational(10),
		   QVariant::fromValue(olive::core::rational(0)));

	// Decreasing time values invert the range: 2 -> 8 and 4 -> 6, which
	// TimeRange normalizes back to [6, 8]
	EXPECT_EQ(remap->InputTimeAdjustment(
				  olive::TimeRemapNode::kInputInput, -1,
				  olive::TimeRange(olive::core::rational(2),
								   olive::core::rational(4)),
				  true),
			  olive::TimeRange(olive::core::rational(6),
							   olive::core::rational(8)));
}

TEST_F(NodeTimeTest, TimeRemapValuePassesConnectedInputThrough)
{
	auto *remap = AddNode<olive::TimeRemapNode>();
	remap->SetInputIsKeyframing(olive::TimeRemapNode::kTimeInput, true);

	AddKey(remap, olive::TimeRemapNode::kTimeInput, olive::core::rational(0),
		   QVariant::fromValue(olive::core::rational(0)));
	AddKey(remap, olive::TimeRemapNode::kTimeInput, olive::core::rational(10),
		   QVariant::fromValue(olive::core::rational(100)));

	auto *time = AddNode<olive::TimeInput>();
	olive::Node::ConnectEdge(
		time, olive::NodeInput(remap, olive::TimeRemapNode::kInputInput));

	// The connected node is evaluated at the remapped time: 3 -> 30
	const olive::NodeValueTable table = GenerateTable(remap, olive::core::rational(3));
	EXPECT_DOUBLE_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 30.0);
}

TEST(TimeFormatNode, Metadata)
{
	olive::TimeFormatNode format;

	EXPECT_EQ(format.Name(), QStringLiteral("Time Format"));
	EXPECT_EQ(format.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.timeformat"));
	EXPECT_FALSE(format.Description().isEmpty());
	EXPECT_TRUE(format.Category().contains(olive::Node::kCategoryGenerator));
}

TEST(TimeFormatNode, RetranslateSetsInputNames)
{
	olive::TimeFormatNode format;

	format.Retranslate();

	EXPECT_EQ(format.GetInputName(olive::TimeFormatNode::kTimeInput),
			  QStringLiteral("Time"));
	EXPECT_EQ(format.GetInputName(olive::TimeFormatNode::kFormatInput),
			  QStringLiteral("Format"));
	EXPECT_EQ(format.GetInputName(olive::TimeFormatNode::kLocalTimeInput),
			  QStringLiteral("Interpret time as local time"));
}

TEST_F(NodeTimeTest, TimeFormatDefaultsToUtcEpoch)
{
	auto *format = AddNode<olive::TimeFormatNode>();

	// Local time interpretation is off by default, keeping output
	// independent of the machine timezone
	EXPECT_FALSE(format->GetStandardValue(olive::TimeFormatNode::kLocalTimeInput)
					 .toBool());
	EXPECT_EQ(format->GetStandardValue(olive::TimeFormatNode::kFormatInput)
				  .toString(),
			  QStringLiteral("hh:mm:ss"));

	// A null time value behaves as 0, the Unix epoch
	const olive::NodeValueTable table = GenerateTable(format, olive::core::rational(0));
	EXPECT_EQ(table.Get(olive::NodeValue::kText).toString(),
			  QStringLiteral("00:00:00"));
}

TEST_F(NodeTimeTest, TimeFormatFormatsUtcTime)
{
	auto *format = AddNode<olive::TimeFormatNode>();
	format->SetStandardValue(olive::TimeFormatNode::kTimeInput, 3661.0);

	const olive::NodeValueTable table = GenerateTable(format, olive::core::rational(0));
	EXPECT_EQ(table.Get(olive::NodeValue::kText).toString(),
			  QStringLiteral("01:01:01"));
}

TEST_F(NodeTimeTest, TimeFormatHonorsCustomFormat)
{
	auto *format = AddNode<olive::TimeFormatNode>();
	format->SetStandardValue(olive::TimeFormatNode::kTimeInput, 3661.0);
	format->SetStandardValue(olive::TimeFormatNode::kFormatInput,
							 QStringLiteral("yyyy-MM-dd mm:ss"));

	// The custom format replaces the default; 3661s = 01:01:01 UTC
	const olive::NodeValueTable table = GenerateTable(format, olive::core::rational(0));
	EXPECT_EQ(table.Get(olive::NodeValue::kText).toString(),
			  QStringLiteral("1970-01-01 01:01"));
}

TEST_F(NodeTimeTest, TimeFormatFormatsNegativeTimeBeforeEpoch)
{
	auto *format = AddNode<olive::TimeFormatNode>();
	format->SetStandardValue(olive::TimeFormatNode::kTimeInput, -1.0);
	format->SetStandardValue(olive::TimeFormatNode::kFormatInput,
							 QStringLiteral("yyyy-MM-dd hh:mm:ss"));

	// One second before the epoch
	const olive::NodeValueTable table = GenerateTable(format, olive::core::rational(0));
	EXPECT_EQ(table.Get(olive::NodeValue::kText).toString(),
			  QStringLiteral("1969-12-31 23:59:59"));
}
