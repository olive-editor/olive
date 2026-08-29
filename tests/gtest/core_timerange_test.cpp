#include <gtest/gtest.h>

#include "olive/core/util/timerange.h"

using namespace olive::core;

TEST(CoreTimeRange, ConstructAndAccess)
{
	TimeRange r(rational(1), rational(5));
	EXPECT_EQ(r.in(), rational(1));
	EXPECT_EQ(r.out(), rational(5));
	EXPECT_EQ(r.length(), rational(4));
}

TEST(CoreTimeRange, NormalizationSwapsReversedBounds)
{
	TimeRange r(rational(5), rational(1));
	EXPECT_EQ(r.in(), rational(1));
	EXPECT_EQ(r.out(), rational(5));
}

TEST(CoreTimeRange, SettersNormalize)
{
	TimeRange r(rational(0), rational(10));
	r.set_in(rational(15));
	EXPECT_EQ(r.in(), rational(10));
	EXPECT_EQ(r.out(), rational(15));

	r.set_out(rational(2));
	EXPECT_EQ(r.in(), rational(2));
	EXPECT_EQ(r.out(), rational(10));
}

TEST(CoreTimeRange, ContainsRational)
{
	TimeRange r(rational(0), rational(10));
	EXPECT_TRUE(r.Contains(rational(5)));
	EXPECT_FALSE(r.Contains(rational(10)));
	EXPECT_FALSE(r.Contains(rational(-1)));
}

TEST(CoreTimeRange, ContainsRange)
{
	TimeRange outer(rational(0), rational(10));
	TimeRange inner(rational(2), rational(8));
	TimeRange partial(rational(5), rational(15));

	EXPECT_TRUE(outer.Contains(inner));
	EXPECT_FALSE(outer.Contains(partial));
}

TEST(CoreTimeRange, OverlapsWith)
{
	TimeRange a(rational(0), rational(10));
	TimeRange b(rational(5), rational(15));
	TimeRange c(rational(10), rational(20));

	EXPECT_TRUE(a.OverlapsWith(b));
	// By default bounds are inclusive, so [0,10] and [10,20] touch and overlap
	EXPECT_TRUE(a.OverlapsWith(c));
	EXPECT_FALSE(a.OverlapsWith(c, false, false));
}

TEST(CoreTimeRange, CombineAndIntersect)
{
	TimeRange a(rational(0), rational(10));
	TimeRange b(rational(5), rational(15));

	TimeRange combined = a.Combined(b);
	EXPECT_EQ(combined.in(), rational(0));
	EXPECT_EQ(combined.out(), rational(15));

	TimeRange intersect = a.Intersected(b);
	EXPECT_EQ(intersect.in(), rational(5));
	EXPECT_EQ(intersect.out(), rational(10));
}

TEST(CoreTimeRange, Arithmetic)
{
	TimeRange r(rational(0), rational(10));
	TimeRange shifted = r + rational(5);
	EXPECT_EQ(shifted.in(), rational(5));
	EXPECT_EQ(shifted.out(), rational(15));

	shifted -= rational(3);
	EXPECT_EQ(shifted.in(), rational(2));
	EXPECT_EQ(shifted.out(), rational(12));
}

TEST(CoreTimeRange, Split)
{
	TimeRange r(rational(0), rational(10));
	auto pieces = r.Split(3);
	ASSERT_EQ(pieces.size(), 4u);
	EXPECT_EQ(pieces.front().in(), rational(0));
}

TEST(CoreTimeRangeList, InsertMergesOverlapping)
{
	TimeRangeList list;
	list.insert(TimeRange(rational(0), rational(5)));
	list.insert(TimeRange(rational(3), rational(8)));
	list.insert(TimeRange(rational(10), rational(12)));

	EXPECT_EQ(list.size(), 2);
	EXPECT_EQ(list.first().in(), rational(0));
	EXPECT_EQ(list.first().out(), rational(8));
}

TEST(CoreTimeRangeList, RemoveSplitsRange)
{
	TimeRangeList list;
	list.insert(TimeRange(rational(0), rational(10)));
	list.remove(TimeRange(rational(3), rational(7)));

	EXPECT_EQ(list.size(), 2);
	EXPECT_EQ(list.first().out(), rational(3));
	EXPECT_EQ(list.last().in(), rational(7));
}

TEST(CoreTimeRangeList, Shift)
{
	TimeRangeList list;
	list.insert(TimeRange(rational(0), rational(5)));
	list.shift(rational(10));

	EXPECT_EQ(list.first().in(), rational(10));
	EXPECT_EQ(list.first().out(), rational(15));
}

TEST(CoreTimeRangeList, TrimInAndOut)
{
	TimeRangeList list;
	list.insert(TimeRange(rational(10), rational(20)));
	list.trim_in(rational(5));
	EXPECT_EQ(list.first().in(), rational(15));
	EXPECT_EQ(list.first().out(), rational(20));

	list.trim_out(rational(-5));
	// set_out(out + diff) = 20 + (-5) = 15
	EXPECT_EQ(list.first().out(), rational(15));
}

TEST(CoreTimeRangeList, Intersects)
{
	TimeRangeList list;
	list.insert(TimeRange(rational(0), rational(10)));
	list.insert(TimeRange(rational(20), rational(30)));

	TimeRangeList result =
		list.Intersects(TimeRange(rational(5), rational(25)));
	EXPECT_EQ(result.size(), 2);
	EXPECT_EQ(result.first().in(), rational(5));
	EXPECT_EQ(result.first().out(), rational(10));
}

TEST(CoreTimeRangeListFrameIterator, IteratesFrames)
{
	TimeRangeList list;
	// 5 seconds at 25fps = 125 frames
	list.insert(TimeRange(rational(0), rational(5)));
	TimeRangeListFrameIterator it(list, rational(1, 25));

	rational out;
	int count = 0;
	while (it.GetNext(&out)) {
		count++;
	}

	EXPECT_EQ(count, 125);
	EXPECT_EQ(it.size(), 125);
}

TEST(CoreTimeRangeListFrameIterator, HasNext)
{
	TimeRangeList list;
	list.insert(TimeRange(rational(0), rational(1)));
	TimeRangeListFrameIterator it(list, rational(1, 25));

	EXPECT_TRUE(it.HasNext());
	rational out;
	while (it.GetNext(&out)) {
	}
	EXPECT_FALSE(it.HasNext());
}
