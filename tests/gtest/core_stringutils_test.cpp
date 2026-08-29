#include <gtest/gtest.h>

#include <regex>

#include "olive/core/util/stringutils.h"

using namespace olive::core;

TEST(CoreStringUtils, Split)
{
	auto result = StringUtils::split("a,b,c", ',');
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[0], "a");
	EXPECT_EQ(result[1], "b");
	EXPECT_EQ(result[2], "c");
}

TEST(CoreStringUtils, SplitRegex)
{
	auto result =
		StringUtils::split_regex("one:two;three", std::regex("(:)|(;)| "));
	ASSERT_EQ(result.size(), 3u);
	EXPECT_EQ(result[0], "one");
	EXPECT_EQ(result[1], "two");
	EXPECT_EQ(result[2], "three");
}

TEST(CoreStringUtils, ToInt)
{
	bool ok = false;
	EXPECT_EQ(StringUtils::to_int("42", &ok), 42);
	EXPECT_TRUE(ok);

	EXPECT_EQ(StringUtils::to_int("-7", 10, &ok), -7);
	EXPECT_TRUE(ok);

	EXPECT_EQ(StringUtils::to_int("ff", 16, &ok), 255);
	EXPECT_TRUE(ok);

	EXPECT_EQ(StringUtils::to_int("abc", &ok), 0);
	EXPECT_FALSE(ok);
}

TEST(CoreStringUtils, ToStringLeftpad)
{
	EXPECT_EQ(StringUtils::to_string_leftpad(5, 3), "005");
	EXPECT_EQ(StringUtils::to_string_leftpad(123, 2), "123");
	EXPECT_EQ(StringUtils::to_string_leftpad(7, 4, '*'), "***7");
}

TEST(CoreStringUtils, Format)
{
	EXPECT_EQ(StringUtils::format("Hello %s %d", "world", 42),
			  "Hello world 42");
}

TEST(CoreStringUtils, Trim)
{
	std::string s = "  hello world  ";
	StringUtils::trim(s);
	EXPECT_EQ(s, "hello world");

	EXPECT_EQ(StringUtils::trimmed("\t\nvalue\t\n"), "value");
	EXPECT_EQ(StringUtils::ltrimmed("  left"), "left");
	EXPECT_EQ(StringUtils::rtrimmed("right  "), "right");
}
