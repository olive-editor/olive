#include <gtest/gtest.h>

#include "timeline/timelinecoordinate.h"

TEST(TimelineCoordinate, DefaultAndSetters)
{
	olive::TimelineCoordinate coord;
	EXPECT_EQ(coord.GetTrack().type(), olive::Track::kNone);
	EXPECT_EQ(coord.GetTrack().index(), 0);

	const olive::core::rational frame(10, 1);
	olive::Track::Reference ref(olive::Track::kVideo, 2);

	coord.SetFrame(frame);
	coord.SetTrack(ref);

	EXPECT_EQ(coord.GetFrame(), frame);
	EXPECT_EQ(coord.GetTrack(), ref);
}

TEST(TimelineCoordinate, Constructors)
{
	const olive::core::rational frame(5, 1);
	olive::Track::Reference ref(olive::Track::kAudio, 1);

	olive::TimelineCoordinate with_ref(frame, ref);
	EXPECT_EQ(with_ref.GetFrame(), frame);
	EXPECT_EQ(with_ref.GetTrack(), ref);

	olive::TimelineCoordinate with_type(frame, olive::Track::kSubtitle, 3);
	EXPECT_EQ(with_type.GetFrame(), frame);
	EXPECT_EQ(with_type.GetTrack().type(), olive::Track::kSubtitle);
	EXPECT_EQ(with_type.GetTrack().index(), 3);
}

TEST(TimelineCoordinate, CopyAndAssignment)
{
	const olive::core::rational frame(7, 1);
	olive::Track::Reference ref(olive::Track::kVideo, 4);

	olive::TimelineCoordinate original(frame, ref);
	olive::TimelineCoordinate copy(original);
	EXPECT_EQ(copy.GetFrame(), frame);
	EXPECT_EQ(copy.GetTrack(), ref);

	olive::TimelineCoordinate assigned;
	assigned = original;
	EXPECT_EQ(assigned.GetFrame(), frame);
	EXPECT_EQ(assigned.GetTrack(), ref);
}

TEST(TimelineCoordinate, Equality)
{
	olive::TimelineCoordinate a(olive::core::rational(5, 1),
								olive::Track::Reference(olive::Track::kVideo,
														1));
	olive::TimelineCoordinate b(olive::core::rational(5, 1),
								olive::Track::Reference(olive::Track::kVideo,
														1));
	olive::TimelineCoordinate c(olive::core::rational(6, 1),
								olive::Track::Reference(olive::Track::kVideo,
														1));
	olive::TimelineCoordinate d(olive::core::rational(5, 1),
								olive::Track::Reference(olive::Track::kAudio,
														1));

	EXPECT_EQ(a.GetFrame(), b.GetFrame());
	EXPECT_EQ(a.GetTrack(), b.GetTrack());
	EXPECT_NE(a.GetFrame(), c.GetFrame());
	EXPECT_NE(a.GetTrack(), d.GetTrack());
}
