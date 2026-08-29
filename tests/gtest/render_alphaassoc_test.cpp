#include <gtest/gtest.h>

#include "render/alphaassoc.h"

TEST(AlphaAssociated, ValuesAreDistinct)
{
	EXPECT_NE(olive::kAlphaNone, olive::kAlphaUnassociated);
	EXPECT_NE(olive::kAlphaNone, olive::kAlphaAssociated);
	EXPECT_NE(olive::kAlphaUnassociated, olive::kAlphaAssociated);
}
