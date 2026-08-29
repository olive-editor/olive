#include <gtest/gtest.h>

#include "common/range.h"

TEST(CommonRange, InRangeExact)
{
	EXPECT_TRUE(InRange(5, 5, 0));
}

TEST(CommonRange, InRangeWithinTolerance)
{
	EXPECT_TRUE(InRange(5.0, 5.5, 1.0));
	EXPECT_TRUE(InRange(5.0, 4.5, 1.0));
}

TEST(CommonRange, OutOfRange)
{
	EXPECT_FALSE(InRange(5.0, 7.0, 1.0));
	EXPECT_FALSE(InRange(5.0, 3.0, 1.0));
}

TEST(CommonRange, BoundaryValues)
{
	EXPECT_TRUE(InRange(5.0, 6.0, 1.0));
	EXPECT_TRUE(InRange(5.0, 4.0, 1.0));
}
