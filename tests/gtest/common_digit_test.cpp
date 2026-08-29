#include <gtest/gtest.h>

#include "common/digit.h"

TEST(CommonDigit, SingleDigit)
{
	EXPECT_EQ(olive::GetDigitCount(0), 1);
	EXPECT_EQ(olive::GetDigitCount(5), 1);
	EXPECT_EQ(olive::GetDigitCount(-5), 1);
}

TEST(CommonDigit, MultipleDigits)
{
	EXPECT_EQ(olive::GetDigitCount(10), 2);
	EXPECT_EQ(olive::GetDigitCount(999), 3);
	EXPECT_EQ(olive::GetDigitCount(1000), 4);
	EXPECT_EQ(olive::GetDigitCount(-12345), 5);
}

TEST(CommonDigit, LargeValue)
{
	EXPECT_EQ(olive::GetDigitCount(123456789012345LL), 15);
}
