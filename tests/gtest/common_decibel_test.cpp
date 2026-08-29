#include <gtest/gtest.h>

#include "common/decibel.h"

TEST(CommonDecibel, FromLinearZeroReturnsMinimum)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::fromLinear(0.0), olive::Decibel::MINIMUM);
}

TEST(CommonDecibel, FromLinearOneReturnsZero)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::fromLinear(1.0), 0.0);
}

TEST(CommonDecibel, FromLinearTenReturnsTwenty)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::fromLinear(10.0), 20.0);
}

TEST(CommonDecibel, ToLinearZeroReturnsOne)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::toLinear(0.0), 1.0);
}

TEST(CommonDecibel, ToLinearMinimumReturnsZero)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::toLinear(olive::Decibel::MINIMUM), 0.0);
}

TEST(CommonDecibel, ToLinearTwentyReturnsTen)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::toLinear(20.0), 10.0);
}

TEST(CommonDecibel, FromLogarithmicAtEdges)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::fromLogarithmic(0.0),
					 olive::Decibel::MINIMUM);
	EXPECT_DOUBLE_EQ(olive::Decibel::fromLogarithmic(1.0), 0.0);
}

TEST(CommonDecibel, ToLogarithmicAtEdges)
{
	EXPECT_DOUBLE_EQ(olive::Decibel::toLogarithmic(0.0), 1.0);
}

TEST(CommonDecibel, LinearLogarithmicRoundTrip)
{
	EXPECT_NEAR(olive::Decibel::LogarithmicToLinear(
					olive::Decibel::LinearToLogarithmic(0.5)),
				0.5, 1e-6);
}
