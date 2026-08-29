#include <gtest/gtest.h>

#include "render/loopmode.h"

TEST(LoopMode, ValuesAreDistinct)
{
	EXPECT_NE(olive::LoopMode::kLoopModeOff, olive::LoopMode::kLoopModeLoop);
	EXPECT_NE(olive::LoopMode::kLoopModeOff, olive::LoopMode::kLoopModeClamp);
	EXPECT_NE(olive::LoopMode::kLoopModeLoop, olive::LoopMode::kLoopModeClamp);
}
