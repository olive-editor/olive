#include <gtest/gtest.h>

#include <memory>

#include <Imath/ImathVec.h>
#include <QObject>
#include <QPointF>
#include <QVector2D>

#include "node/color/colormanager/colormanager.h"
#include "node/generator/matrix/matrix.h"
#include "node/generator/solid/solid.h"
#include "node/inputimmediate.h"
#include "node/keyframe.h"
#include "node/math/math/math.h"
#include "node/node.h"
#include "node/project.h"
#include "node/time/timeoffset/timeoffsetnode.h"
#include "olive/core/util/bezier.h"
#include "undo/undocommand.h"

namespace
{

// NodeInputImmediate is not a QObject, so keyframes inserted directly into it
// (rather than through parenting to a Node) must be removed and deleted by
// hand to avoid leaks.
void ClearImmediate(olive::NodeInputImmediate *imm)
{
	for (int i = 0; i < imm->keyframe_tracks().size(); i++) {
		const QVector<olive::NodeKeyframe *> keys = imm->keyframe_tracks().at(i);
		for (olive::NodeKeyframe *key : keys) {
			imm->remove_keyframe(key);
			delete key;
		}
	}
}

olive::NodeKeyframe *MakeKey(const olive::rational &time, const QVariant &value,
							 int track,
							 olive::NodeKeyframe::Type type =
								 olive::NodeKeyframe::kLinear)
{
	return new olive::NodeKeyframe(time, value, type, track, -1,
								   QStringLiteral("test_in"));
}

} // namespace

TEST(NodeInputImmediate, StandardValueRoundTrip)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kFloat, { 1.5 });

	EXPECT_FALSE(imm.is_keyframing());
	EXPECT_TRUE(imm.is_using_standard_value(0));

	// The default value seeds the standard value
	ASSERT_EQ(imm.get_split_standard_value().size(), 1);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value().at(0).toDouble(), 1.5);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(0).toDouble(), 1.5);

	imm.set_split_standard_value({ 2.5 });
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value().at(0).toDouble(), 2.5);

	imm.set_standard_value_on_track(3.5, 0);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(0).toDouble(), 3.5);
}

TEST(NodeInputImmediate, SetSplitStandardValueCopiesOnlyOverlappingTracks)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kVec2, { 0.0, 0.0 });
	ASSERT_EQ(imm.get_split_standard_value().size(), 2);

	// A shorter split only overwrites the tracks it covers
	imm.set_split_standard_value({ 5.0 });
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(0).toDouble(), 5.0);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(1).toDouble(), 0.0);

	// A longer split is clamped to the existing track count
	imm.set_split_standard_value({ 1.0, 2.0, 3.0 });
	ASSERT_EQ(imm.get_split_standard_value().size(), 2);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(0).toDouble(), 1.0);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(1).toDouble(), 2.0);
}

TEST(NodeInputImmediate, SetDataTypeResizesTracksAndReappliesDefault)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kFloat, { 7.0 });
	ASSERT_EQ(imm.keyframe_tracks().size(), 1);
	ASSERT_EQ(imm.get_split_standard_value().size(), 1);

	// Growing to a four-track type keeps the default on the first track and
	// leaves the new tracks null, since the default split only has one entry
	imm.set_data_type(olive::NodeValue::kVec4);
	EXPECT_EQ(imm.keyframe_tracks().size(), 4);
	ASSERT_EQ(imm.get_split_standard_value().size(), 4);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(0).toDouble(), 7.0);
	EXPECT_TRUE(imm.get_split_standard_value_on_track(1).isNull());
	EXPECT_TRUE(imm.get_split_standard_value_on_track(2).isNull());
	EXPECT_TRUE(imm.get_split_standard_value_on_track(3).isNull());

	imm.set_data_type(olive::NodeValue::kFloat);
	EXPECT_EQ(imm.keyframe_tracks().size(), 1);
	ASSERT_EQ(imm.get_split_standard_value().size(), 1);
	EXPECT_DOUBLE_EQ(imm.get_split_standard_value_on_track(0).toDouble(), 7.0);
}

TEST(NodeInputImmediate, KeyframeLookupRequiresKeyframingEnabled)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kFloat, { 0.0 });

	olive::NodeKeyframe *key = MakeKey(olive::rational(2), 1.0, 0);
	imm.insert_keyframe(key);

	// Without keyframing enabled the track reports that it uses the standard
	// value and all keyframe lookups come back empty
	EXPECT_TRUE(imm.is_using_standard_value(0));
	EXPECT_FALSE(imm.has_keyframe_at_time(olive::rational(2)));
	EXPECT_EQ(imm.get_keyframe_at_time_on_track(olive::rational(2), 0),
			  nullptr);
	EXPECT_TRUE(imm.get_keyframe_at_time(olive::rational(2)).isEmpty());
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(2), 0),
			  nullptr);

	imm.set_is_keyframing(true);
	EXPECT_FALSE(imm.is_using_standard_value(0));
	EXPECT_TRUE(imm.has_keyframe_at_time(olive::rational(2)));
	EXPECT_EQ(imm.get_keyframe_at_time_on_track(olive::rational(2), 0), key);
	ASSERT_EQ(imm.get_keyframe_at_time(olive::rational(2)).size(), 1);
	EXPECT_EQ(imm.get_keyframe_at_time(olive::rational(2)).first(), key);
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(2), 0),
			  key);
	EXPECT_FALSE(imm.has_keyframe_at_time(olive::rational(3)));

	ClearImmediate(&imm);
}

TEST(NodeInputImmediate, InsertKeyframeSortsByTimeAndLinksSiblings)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kFloat, { 0.0 });

	// Insert out of order; the track must stay sorted by time
	olive::NodeKeyframe *key_late = MakeKey(olive::rational(10), 10.0, 0);
	olive::NodeKeyframe *key_early = MakeKey(olive::rational(0), 0.0, 0);
	olive::NodeKeyframe *key_mid = MakeKey(olive::rational(5), 5.0, 0);
	imm.insert_keyframe(key_late);
	imm.insert_keyframe(key_early);
	imm.insert_keyframe(key_mid);

	const olive::NodeKeyframeTrack &track = imm.keyframe_tracks().at(0);
	ASSERT_EQ(track.size(), 3);
	EXPECT_EQ(track.at(0), key_early);
	EXPECT_EQ(track.at(1), key_mid);
	EXPECT_EQ(track.at(2), key_late);

	// Sibling links follow the sorted order
	EXPECT_EQ(key_early->previous(), nullptr);
	EXPECT_EQ(key_early->next(), key_mid);
	EXPECT_EQ(key_mid->previous(), key_early);
	EXPECT_EQ(key_mid->next(), key_late);
	EXPECT_EQ(key_late->previous(), key_mid);
	EXPECT_EQ(key_late->next(), nullptr);

	EXPECT_EQ(imm.get_earliest_keyframe(), key_early);
	EXPECT_EQ(imm.get_latest_keyframe(), key_late);

	// Removing the middle keyframe re-links its siblings
	imm.remove_keyframe(key_mid);
	EXPECT_EQ(key_early->next(), key_late);
	EXPECT_EQ(key_late->previous(), key_early);
	EXPECT_EQ(key_mid->previous(), nullptr);
	EXPECT_EQ(key_mid->next(), nullptr);
	ASSERT_EQ(track.size(), 2);
	delete key_mid;

	ClearImmediate(&imm);
}

TEST(NodeInputImmediate, ClosestKeyframeToTimeOnTrackClampsAndPicksNearest)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kVec2, { 0.0, 0.0 });
	imm.set_is_keyframing(true);

	olive::NodeKeyframe *key_a = MakeKey(olive::rational(0), 0.0, 0);
	olive::NodeKeyframe *key_b = MakeKey(olive::rational(10), 10.0, 0);
	imm.insert_keyframe(key_a);
	imm.insert_keyframe(key_b);

	// Outside the keyed range the closest keyframe clamps to the ends
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(-3), 0),
			  key_a);
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(20), 0),
			  key_b);

	// Between the keys the nearer one wins
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(3), 0),
			  key_a);
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(7), 0),
			  key_b);

	// Exactly halfway the earlier keyframe wins the tie
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(5), 0),
			  key_a);

	// A track with no keyframes still counts as using the standard value
	EXPECT_EQ(imm.get_closest_keyframe_to_time_on_track(olive::rational(5), 1),
			  nullptr);

	ClearImmediate(&imm);
}

TEST(NodeInputImmediate, ClosestKeyframeBeforeAfterSpansAllTracks)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kVec2, { 0.0, 0.0 });
	imm.set_is_keyframing(true);

	olive::NodeKeyframe *key_t0 = MakeKey(olive::rational(0), 0.0, 0);
	olive::NodeKeyframe *key_t10 = MakeKey(olive::rational(10), 10.0, 0);
	olive::NodeKeyframe *key_t4_track1 = MakeKey(olive::rational(4), 4.0, 1);
	imm.insert_keyframe(key_t0);
	imm.insert_keyframe(key_t10);
	imm.insert_keyframe(key_t4_track1);

	// The closest keyframe before 5 is the one at 4 on the other track
	EXPECT_EQ(imm.get_closest_keyframe_before_time(olive::rational(5)),
			  key_t4_track1);
	EXPECT_EQ(imm.get_closest_keyframe_after_time(olive::rational(5)), key_t10);

	// Strictly before/after: nothing exists outside the keyed range
	EXPECT_EQ(imm.get_closest_keyframe_before_time(olive::rational(0)),
			  nullptr);
	EXPECT_EQ(imm.get_closest_keyframe_after_time(olive::rational(10)),
			  nullptr);

	EXPECT_EQ(imm.get_closest_keyframe_before_time(olive::rational(4)), key_t0);
	EXPECT_EQ(imm.get_closest_keyframe_after_time(olive::rational(4)), key_t10);

	ClearImmediate(&imm);
}

TEST(NodeInputImmediate, BestKeyframeTypeForTimeFollowsClosestKey)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kFloat, { 0.0 });

	// With no keyframes there is no reference, so the default type is used
	EXPECT_EQ(int(imm.get_best_keyframe_type_for_time(olive::rational(5), 0)),
			  int(olive::NodeKeyframe::kDefaultType));

	olive::NodeKeyframe *key_hold =
		MakeKey(olive::rational(0), 0.0, 0, olive::NodeKeyframe::kHold);
	olive::NodeKeyframe *key_linear = MakeKey(olive::rational(10), 10.0, 0);
	imm.insert_keyframe(key_hold);
	imm.insert_keyframe(key_linear);
	imm.set_is_keyframing(true);

	EXPECT_EQ(int(imm.get_best_keyframe_type_for_time(olive::rational(2), 0)),
			  int(olive::NodeKeyframe::kHold));
	EXPECT_EQ(int(imm.get_best_keyframe_type_for_time(olive::rational(8), 0)),
			  int(olive::NodeKeyframe::kLinear));

	ClearImmediate(&imm);
}

TEST(NodeInputImmediate, GetKeyframeAtTimeAggregatesAcrossTracks)
{
	olive::NodeInputImmediate imm(olive::NodeValue::kVec2, { 0.0, 0.0 });
	imm.set_is_keyframing(true);

	olive::NodeKeyframe *key_track0 = MakeKey(olive::rational(3), 1.0, 0);
	olive::NodeKeyframe *key_track1 = MakeKey(olive::rational(3), 2.0, 1);
	olive::NodeKeyframe *key_later = MakeKey(olive::rational(7), 3.0, 0);
	imm.insert_keyframe(key_track0);
	imm.insert_keyframe(key_track1);
	imm.insert_keyframe(key_later);

	// Both tracks have a keyframe at t=3
	QVector<olive::NodeKeyframe *> at_three =
		imm.get_keyframe_at_time(olive::rational(3));
	ASSERT_EQ(at_three.size(), 2);
	EXPECT_TRUE(at_three.contains(key_track0));
	EXPECT_TRUE(at_three.contains(key_track1));

	// Only track 0 has one at t=7, and there is nothing at t=99
	EXPECT_EQ(imm.get_keyframe_at_time(olive::rational(7)).size(), 1);
	EXPECT_TRUE(imm.get_keyframe_at_time(olive::rational(99)).isEmpty());

	ClearImmediate(&imm);
}

class NodeInputImmediateNodeTest : public ::testing::Test {
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
								const olive::rational &time,
								const QVariant &value, int track,
								olive::NodeKeyframe::Type type =
									olive::NodeKeyframe::kLinear)
	{
		auto *key = new olive::NodeKeyframe(time, value, type, track, -1, input);
		key->setParent(node);
		return key;
	}

	std::unique_ptr<olive::Project> project_;
};

TEST_F(NodeInputImmediateNodeTest, SetValueAtTimeCreatesAndUpdatesKeyframes)
{
	auto *node = AddNode<olive::MathNode>();
	const olive::NodeInput input(node, olive::MathNode::kParamAIn);
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	// A new keyframe is inserted where none exists yet
	olive::MultiUndoCommand cmd;
	olive::Node::SetValueAtTime(input, olive::rational(5), 42.0, 0, &cmd, true);
	EXPECT_EQ(cmd.child_count(), 1);
	cmd.redo_now();

	olive::NodeKeyframe *key = node->GetKeyframeAtTimeOnTrack(
		olive::MathNode::kParamAIn, olive::rational(5), 0);
	ASSERT_NE(key, nullptr);
	EXPECT_DOUBLE_EQ(key->value().toDouble(), 42.0);
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(5))
						 .toDouble(),
					 42.0);

	// Setting the same time again updates the existing keyframe in place
	olive::MultiUndoCommand update_cmd;
	olive::Node::SetValueAtTime(input, olive::rational(5), 43.0, 0,
								&update_cmd, true);
	EXPECT_EQ(update_cmd.child_count(), 1);
	update_cmd.redo_now();

	const QVector<olive::NodeKeyframeTrack> &tracks =
		node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1);
	ASSERT_EQ(tracks.at(0).size(), 1);
	EXPECT_EQ(tracks.at(0).first(), key);
	EXPECT_DOUBLE_EQ(key->value().toDouble(), 43.0);
}

TEST_F(NodeInputImmediateNodeTest, SetValueAtTimeWithoutKeyframingSetsStandardValue)
{
	auto *node = AddNode<olive::MathNode>();
	const olive::NodeInput input(node, olive::MathNode::kParamAIn);

	olive::MultiUndoCommand cmd;
	olive::Node::SetValueAtTime(input, olive::rational(5), 9.0, 0, &cmd, true);
	EXPECT_EQ(cmd.child_count(), 1);
	cmd.redo_now();

	EXPECT_DOUBLE_EQ(
		node->GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 9.0);
	EXPECT_TRUE(node->GetKeyframeTracks(olive::MathNode::kParamAIn, -1)
					.at(0)
					.isEmpty());
}

TEST_F(NodeInputImmediateNodeTest, SetValueAtTimeInsertsOnAllTracksOnlyWhenAsked)
{
	auto *solid = AddNode<olive::SolidGenerator>();
	solid->SetInputIsKeyframing(olive::SolidGenerator::kColorInput, true);
	const olive::NodeInput input(solid, olive::SolidGenerator::kColorInput);

	// With insert_on_all_tracks_if_no_key set, keyframes are created on every
	// track; sibling tracks capture the value they currently evaluate to (the
	// standard value red = (1, 0, 0, 1) here)
	olive::MultiUndoCommand cmd;
	olive::Node::SetValueAtTime(input, olive::rational(5), 0.5, 2, &cmd, true);
	EXPECT_EQ(cmd.child_count(), 4);
	cmd.redo_now();

	const QVector<olive::NodeKeyframeTrack> &tracks =
		solid->GetKeyframeTracks(olive::SolidGenerator::kColorInput, -1);
	ASSERT_EQ(tracks.size(), 4);
	for (int i = 0; i < tracks.size(); i++) {
		ASSERT_EQ(tracks.at(i).size(), 1);
		EXPECT_EQ(tracks.at(i).first()->time(), olive::rational(5));
	}
	EXPECT_DOUBLE_EQ(tracks.at(0).first()->value().toDouble(), 1.0);
	EXPECT_DOUBLE_EQ(tracks.at(1).first()->value().toDouble(), 0.0);
	EXPECT_DOUBLE_EQ(tracks.at(2).first()->value().toDouble(), 0.5);
	EXPECT_DOUBLE_EQ(tracks.at(3).first()->value().toDouble(), 1.0);

	const olive::Color c =
		solid->GetValueAtTime(olive::SolidGenerator::kColorInput,
							  olive::rational(5))
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(c.red(), 1.0f);
	EXPECT_FLOAT_EQ(c.green(), 0.0f);
	EXPECT_FLOAT_EQ(c.blue(), 0.5f);
	EXPECT_FLOAT_EQ(c.alpha(), 1.0f);

	// Without the flag only the requested track receives a keyframe
	auto *single = AddNode<olive::SolidGenerator>();
	single->SetInputIsKeyframing(olive::SolidGenerator::kColorInput, true);
	const olive::NodeInput single_input(single,
										olive::SolidGenerator::kColorInput);

	olive::MultiUndoCommand single_cmd;
	olive::Node::SetValueAtTime(single_input, olive::rational(5), 0.5, 2,
								&single_cmd, false);
	EXPECT_EQ(single_cmd.child_count(), 1);
	single_cmd.redo_now();

	const QVector<olive::NodeKeyframeTrack> &single_tracks =
		single->GetKeyframeTracks(olive::SolidGenerator::kColorInput, -1);
	ASSERT_EQ(single_tracks.size(), 4);
	EXPECT_TRUE(single_tracks.at(0).isEmpty());
	EXPECT_TRUE(single_tracks.at(1).isEmpty());
	ASSERT_EQ(single_tracks.at(2).size(), 1);
	EXPECT_DOUBLE_EQ(single_tracks.at(2).first()->value().toDouble(), 0.5);
	EXPECT_TRUE(single_tracks.at(3).isEmpty());
}

TEST_F(NodeInputImmediateNodeTest, GetValueAtTimeInterpolatesColorTracks)
{
	auto *solid = AddNode<olive::SolidGenerator>();
	solid->SetInputIsKeyframing(olive::SolidGenerator::kColorInput, true);

	// Black to white over ten seconds on all four tracks
	for (int track = 0; track < 4; track++) {
		AddKey(solid, olive::SolidGenerator::kColorInput, olive::rational(0),
			   0.0, track);
		AddKey(solid, olive::SolidGenerator::kColorInput, olive::rational(10),
			   1.0, track);
	}

	const olive::Color mid =
		solid->GetValueAtTime(olive::SolidGenerator::kColorInput,
							  olive::rational(5))
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(mid.red(), 0.5f);
	EXPECT_FLOAT_EQ(mid.green(), 0.5f);
	EXPECT_FLOAT_EQ(mid.blue(), 0.5f);
	EXPECT_FLOAT_EQ(mid.alpha(), 0.5f);

	const olive::SplitValue split = solid->GetSplitValueAtTime(
		olive::SolidGenerator::kColorInput, olive::rational(5));
	ASSERT_EQ(split.size(), 4);
	for (int i = 0; i < split.size(); i++) {
		EXPECT_DOUBLE_EQ(split.at(i).toDouble(), 0.5);
	}

	// Outside the keyed range the end values hold
	const olive::Color before =
		solid->GetValueAtTime(olive::SolidGenerator::kColorInput,
							  olive::rational(-2))
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(before.red(), 0.0f);
	const olive::Color after =
		solid->GetValueAtTime(olive::SolidGenerator::kColorInput,
							  olive::rational(20))
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(after.alpha(), 1.0f);
}

TEST_F(NodeInputImmediateNodeTest, GetValueAtTimeInterpolatesVec2Tracks)
{
	auto *matrix = AddNode<olive::MatrixGenerator>();
	matrix->SetInputIsKeyframing(olive::MatrixGenerator::kPositionInput, true);

	AddKey(matrix, olive::MatrixGenerator::kPositionInput, olive::rational(0),
		   0.0, 0);
	AddKey(matrix, olive::MatrixGenerator::kPositionInput, olive::rational(10),
		   10.0, 0);
	AddKey(matrix, olive::MatrixGenerator::kPositionInput, olive::rational(0),
		   10.0, 1);
	AddKey(matrix, olive::MatrixGenerator::kPositionInput, olive::rational(10),
		   20.0, 1);

	const QVector2D mid =
		matrix->GetValueAtTime(olive::MatrixGenerator::kPositionInput,
							   olive::rational(5))
			.value<QVector2D>();
	EXPECT_FLOAT_EQ(mid.x(), 5.0f);
	EXPECT_FLOAT_EQ(mid.y(), 15.0f);

	// Each track clamps to its own end keyframes
	const QVector2D clamped_low =
		matrix->GetValueAtTime(olive::MatrixGenerator::kPositionInput,
							   olive::rational(-5))
			.value<QVector2D>();
	EXPECT_FLOAT_EQ(clamped_low.x(), 0.0f);
	EXPECT_FLOAT_EQ(clamped_low.y(), 10.0f);
	const QVector2D clamped_high =
		matrix->GetValueAtTime(olive::MatrixGenerator::kPositionInput,
							   olive::rational(15))
			.value<QVector2D>();
	EXPECT_FLOAT_EQ(clamped_high.x(), 10.0f);
	EXPECT_FLOAT_EQ(clamped_high.y(), 20.0f);
}

TEST_F(NodeInputImmediateNodeTest, GetValueAtTimeInterpolatesRationalAsRational)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetInputIsKeyframing(olive::TimeOffsetNode::kTimeInput, true);

	AddKey(offset, olive::TimeOffsetNode::kTimeInput, olive::rational(0),
		   QVariant::fromValue(olive::rational(0)), 0);
	AddKey(offset, olive::TimeOffsetNode::kTimeInput, olive::rational(10),
		   QVariant::fromValue(olive::rational(10)), 0);

	// The interpolated value is converted back into a rational
	const QVariant mid = offset->GetValueAtTime(
		olive::TimeOffsetNode::kTimeInput, olive::rational(5));
	EXPECT_EQ(mid.value<olive::rational>(), olive::rational(5));

	const QVariant one_tenth_in = offset->GetValueAtTime(
		olive::TimeOffsetNode::kTimeInput, olive::rational(1));
	EXPECT_DOUBLE_EQ(one_tenth_in.value<olive::rational>().toDouble(), 1.0);
}

TEST_F(NodeInputImmediateNodeTest, GetValueAtTimeHoldsRationalUntilNextKey)
{
	auto *offset = AddNode<olive::TimeOffsetNode>();
	offset->SetInputIsKeyframing(olive::TimeOffsetNode::kTimeInput, true);

	AddKey(offset, olive::TimeOffsetNode::kTimeInput, olive::rational(0),
		   QVariant::fromValue(olive::rational(2, 3)), 0,
		   olive::NodeKeyframe::kHold);
	AddKey(offset, olive::TimeOffsetNode::kTimeInput, olive::rational(10),
		   QVariant::fromValue(olive::rational(4, 3)), 0);

	// A hold keyframe keeps its exact rational value until the next key
	const QVariant held = offset->GetValueAtTime(
		olive::TimeOffsetNode::kTimeInput, olive::rational(9));
	EXPECT_EQ(held.value<olive::rational>(), olive::rational(2, 3));

	const QVariant at_next = offset->GetValueAtTime(
		olive::TimeOffsetNode::kTimeInput, olive::rational(10));
	EXPECT_EQ(at_next.value<olive::rational>(), olive::rational(4, 3));
}

TEST_F(NodeInputImmediateNodeTest, GetValueAtTimeBezierHandlesBendCurve)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	olive::NodeKeyframe *before =
		AddKey(node, olive::MathNode::kParamAIn, olive::rational(0), 0.0, 0,
			   olive::NodeKeyframe::kBezier);
	olive::NodeKeyframe *after =
		AddKey(node, olive::MathNode::kParamAIn, olive::rational(10), 10.0, 0,
			   olive::NodeKeyframe::kBezier);

	// Ease-in shape: the outgoing handle pulls the start of the curve flat
	before->set_bezier_control_out(QPointF(2.5, 0.0));
	after->set_bezier_control_in(QPointF(0.0, 0.0));

	const double interpolated = node->GetValueAtTime(
		olive::MathNode::kParamAIn, olive::rational(5)).toDouble();
	const double expected = olive::core::Bezier::CubicXtoY(
		5.0, Imath::V2d(0.0, 0.0), Imath::V2d(2.5, 0.0), Imath::V2d(10.0, 10.0),
		Imath::V2d(10.0, 10.0));
	EXPECT_DOUBLE_EQ(interpolated, expected);

	// The ease-in handle keeps the midpoint below the linear value of 5.0
	EXPECT_LT(interpolated, 5.0);
}

TEST_F(NodeInputImmediateNodeTest, GetValueAtTimeQuadraticBezierWithOneHandle)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	// Bezier into linear uses a quadratic curve with a single control point
	olive::NodeKeyframe *before =
		AddKey(node, olive::MathNode::kParamAIn, olive::rational(0), 0.0, 0,
			   olive::NodeKeyframe::kBezier);
	AddKey(node, olive::MathNode::kParamAIn, olive::rational(10), 10.0, 0,
		   olive::NodeKeyframe::kLinear);
	before->set_bezier_control_out(QPointF(2.5, 0.0));

	const double interpolated = node->GetValueAtTime(
		olive::MathNode::kParamAIn, olive::rational(5)).toDouble();
	const double expected = olive::core::Bezier::QuadraticXtoY(
		5.0, Imath::V2d(0.0, 0.0), Imath::V2d(2.5, 0.0),
		Imath::V2d(10.0, 10.0));
	EXPECT_DOUBLE_EQ(interpolated, expected);
	EXPECT_LT(interpolated, 5.0);
}

TEST_F(NodeInputImmediateNodeTest, IsUsingStandardValueTransitions)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetStandardValue(olive::MathNode::kParamAIn, 3.0);

	// Static input: standard value is always in use
	EXPECT_TRUE(node->IsUsingStandardValue(olive::MathNode::kParamAIn, 0));

	// Keyframing enabled but no keyframes yet: still the standard value
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);
	EXPECT_TRUE(node->IsUsingStandardValue(olive::MathNode::kParamAIn, 0));

	// With a keyframe present the track switches to the keyed value
	olive::NodeKeyframe *key =
		AddKey(node, olive::MathNode::kParamAIn, olive::rational(5), 7.0, 0);
	EXPECT_FALSE(node->IsUsingStandardValue(olive::MathNode::kParamAIn, 0));

	// Disabling keyframing hides the keyframes again
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, false);
	EXPECT_TRUE(node->IsUsingStandardValue(olive::MathNode::kParamAIn, 0));
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(5))
						 .toDouble(),
					 3.0);

	// Removing the last keyframe returns the track to the standard value
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);
	EXPECT_FALSE(node->IsUsingStandardValue(olive::MathNode::kParamAIn, 0));
	key->setParent(nullptr);
	delete key;
	EXPECT_TRUE(node->IsUsingStandardValue(olive::MathNode::kParamAIn, 0));
	EXPECT_DOUBLE_EQ(node->GetValueAtTime(olive::MathNode::kParamAIn,
										  olive::rational(5))
						 .toDouble(),
					 3.0);
}

TEST_F(NodeInputImmediateNodeTest, PartiallyKeyedTrackFallsBackToStandardValue)
{
	auto *matrix = AddNode<olive::MatrixGenerator>();
	matrix->SetStandardValue(olive::MatrixGenerator::kPositionInput,
							 QVector2D(1.0f, 2.0f));
	matrix->SetInputIsKeyframing(olive::MatrixGenerator::kPositionInput, true);

	// Only the X track is keyed; the Y track keeps its standard value
	AddKey(matrix, olive::MatrixGenerator::kPositionInput, olive::rational(0),
		   0.0, 0);
	AddKey(matrix, olive::MatrixGenerator::kPositionInput, olive::rational(10),
		   10.0, 0);

	EXPECT_FALSE(
		matrix->IsUsingStandardValue(olive::MatrixGenerator::kPositionInput,
									 0));
	EXPECT_TRUE(
		matrix->IsUsingStandardValue(olive::MatrixGenerator::kPositionInput,
									 1));

	const QVector2D value =
		matrix->GetValueAtTime(olive::MatrixGenerator::kPositionInput,
							   olive::rational(5))
			.value<QVector2D>();
	EXPECT_FLOAT_EQ(value.x(), 5.0f);
	EXPECT_FLOAT_EQ(value.y(), 2.0f);
}

TEST_F(NodeInputImmediateNodeTest, StandardValueCombinationAcrossTracks)
{
	auto *solid = AddNode<olive::SolidGenerator>();

	// The declared default is opaque red
	olive::Color initial =
		solid->GetStandardValue(olive::SolidGenerator::kColorInput)
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(initial.red(), 1.0f);
	EXPECT_FLOAT_EQ(initial.green(), 0.0f);
	EXPECT_FLOAT_EQ(initial.blue(), 0.0f);
	EXPECT_FLOAT_EQ(initial.alpha(), 1.0f);

	// Setting a normal value splits it across the four tracks
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.5f, 0.75f, 1.0f)));
	const olive::SplitValue split =
		solid->GetSplitStandardValue(olive::SolidGenerator::kColorInput);
	ASSERT_EQ(split.size(), 4);
	EXPECT_DOUBLE_EQ(split.at(0).toDouble(), 0.25);
	EXPECT_DOUBLE_EQ(split.at(1).toDouble(), 0.5);
	EXPECT_DOUBLE_EQ(split.at(2).toDouble(), 0.75);
	EXPECT_DOUBLE_EQ(split.at(3).toDouble(), 1.0);
	EXPECT_DOUBLE_EQ(solid->GetSplitStandardValueOnTrack(
						 olive::SolidGenerator::kColorInput, 2)
						 .toDouble(),
					 0.75);

	// A partial split only overwrites the leading tracks
	solid->SetSplitStandardValue(olive::SolidGenerator::kColorInput,
								 { 0.1, 0.2 });
	const olive::Color combined =
		solid->GetStandardValue(olive::SolidGenerator::kColorInput)
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(combined.red(), 0.1f);
	EXPECT_FLOAT_EQ(combined.green(), 0.2f);
	EXPECT_FLOAT_EQ(combined.blue(), 0.75f);
	EXPECT_FLOAT_EQ(combined.alpha(), 1.0f);
}

TEST_F(NodeInputImmediateNodeTest, DeleteAllKeyframesReparentsOrDeletes)
{
	auto *node = AddNode<olive::MathNode>();
	node->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	olive::NodeKeyframe *key_a =
		AddKey(node, olive::MathNode::kParamAIn, olive::rational(0), 1.0, 0);
	olive::NodeKeyframe *key_b =
		AddKey(node, olive::MathNode::kParamAIn, olive::rational(1), 2.0, 0);

	olive::NodeInputImmediate *imm =
		node->GetImmediate(olive::MathNode::kParamAIn, -1);
	ASSERT_NE(imm, nullptr);
	ASSERT_EQ(imm->keyframe_tracks().at(0).size(), 2);

	// With a parent the keyframes are handed over rather than deleted
	QObject guard;
	imm->delete_all_keyframes(&guard);
	EXPECT_TRUE(imm->keyframe_tracks().at(0).isEmpty());
	EXPECT_EQ(guard.children().size(), 2);
	EXPECT_TRUE(guard.children().contains(key_a));
	EXPECT_TRUE(guard.children().contains(key_b));

	// Without a parent the keyframes are deleted outright
	key_a->setParent(node);
	key_b->setParent(node);
	ASSERT_EQ(imm->keyframe_tracks().at(0).size(), 2);
	imm->delete_all_keyframes();
	EXPECT_TRUE(imm->keyframe_tracks().at(0).isEmpty());
}
