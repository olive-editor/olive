#include <gtest/gtest.h>

#include "common/jobtime.h"

TEST(CommonJobTime, ConstructorAcquiresValue)
{
	olive::JobTime a;
	olive::JobTime b;

	EXPECT_NE(a.value(), b.value());
	EXPECT_LT(a.value(), b.value());
}

TEST(CommonJobTime, AcquireUpdatesValue)
{
	olive::JobTime a;
	uint64_t first = a.value();
	a.Acquire();
	uint64_t second = a.value();

	EXPECT_GT(second, first);
}

TEST(CommonJobTime, ComparisonOperators)
{
	olive::JobTime a;
	olive::JobTime b;

	EXPECT_LT(a, b);
	EXPECT_GT(b, a);
	EXPECT_LE(a, a);
	EXPECT_GE(b, b);
	EXPECT_EQ(a, a);
	EXPECT_NE(a, b);
}

TEST(CommonJobTime, DebugStream)
{
	olive::JobTime a;
	QDebug debug(QtDebugMsg);
	debug << a;
}
