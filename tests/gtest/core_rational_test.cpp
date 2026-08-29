#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <sstream>

#include "olive/core/util/rational.h"

using namespace olive::core;

TEST(CoreRational, DefaultConstruction)
{
	rational r;
	EXPECT_EQ(r.numerator(), 0);
	EXPECT_EQ(r.denominator(), 1);
}

TEST(CoreRational, IntegerConstruction)
{
	rational r(5);
	EXPECT_EQ(r.numerator(), 5);
	EXPECT_EQ(r.denominator(), 1);
}

TEST(CoreRational, FractionConstructionReduces)
{
	rational r(4, 8);
	EXPECT_EQ(r.numerator(), 1);
	EXPECT_EQ(r.denominator(), 2);
}

TEST(CoreRational, NegativeDenominatorNormalizes)
{
	rational r(1, -2);
	EXPECT_EQ(r.numerator(), -1);
	EXPECT_EQ(r.denominator(), 2);
}

TEST(CoreRational, ZeroNormalizes)
{
	rational r(0, 5);
	EXPECT_EQ(r.numerator(), 0);
	EXPECT_EQ(r.denominator(), 1);
}

TEST(CoreRational, FromDouble)
{
	bool ok = false;
	rational r = rational::fromDouble(0.5, &ok);
	EXPECT_TRUE(ok);
	EXPECT_EQ(r, rational(1, 2));

	r = rational::fromDouble(std::numeric_limits<double>::quiet_NaN(), &ok);
	EXPECT_FALSE(ok);
	EXPECT_TRUE(r.isNaN());
}

TEST(CoreRational, FromString)
{
	bool ok = false;
	rational r = rational::fromString("3/4", &ok);
	EXPECT_TRUE(ok);
	EXPECT_EQ(r, rational(3, 4));

	r = rational::fromString("42", &ok);
	EXPECT_TRUE(ok);
	EXPECT_EQ(r, rational(42));

	r = rational::fromString("1/2/3", &ok);
	EXPECT_FALSE(ok);
	EXPECT_TRUE(r.isNaN());
}

TEST(CoreRational, ToDouble)
{
	EXPECT_DOUBLE_EQ(rational(1, 2).toDouble(), 0.5);
	EXPECT_TRUE(std::isnan(rational::NaN.toDouble()));
}

TEST(CoreRational, ToString)
{
	EXPECT_EQ(rational(1, 2).toString(), "1/2");
}

TEST(CoreRational, Arithmetic)
{
	rational a(1, 2);
	rational b(1, 3);

	EXPECT_EQ(a + b, rational(5, 6));
	EXPECT_EQ(a - b, rational(1, 6));
	EXPECT_EQ(a * b, rational(1, 6));
	EXPECT_EQ(a / b, rational(3, 2));
}

TEST(CoreRational, CompoundAssignment)
{
	rational a(1, 2);
	a += rational(1, 4);
	EXPECT_EQ(a, rational(3, 4));

	a *= rational(2, 3);
	EXPECT_EQ(a, rational(1, 2));
}

TEST(CoreRational, Comparisons)
{
	rational a(1, 2);
	rational b(2, 4);
	rational c(1, 3);

	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
	EXPECT_TRUE(a > c);
	EXPECT_TRUE(c < a);
	EXPECT_TRUE(a >= b);
	EXPECT_TRUE(c <= a);
}

TEST(CoreRational, UnaryOperators)
{
	rational a(1, 2);
	EXPECT_EQ(+a, a);
	EXPECT_EQ(-a, rational(-1, 2));
	EXPECT_FALSE(!a);

	rational zero(0);
	EXPECT_TRUE(!zero);
}

TEST(CoreRational, Flip)
{
	rational a(2, 3);
	a.flip();
	EXPECT_EQ(a, rational(3, 2));

	rational zero(0);
	zero.flip();
	EXPECT_EQ(zero, rational(0));
}

TEST(CoreRational, Flipped)
{
	EXPECT_EQ(rational(2, 3).flipped(), rational(3, 2));
}

TEST(CoreRational, IsNullAndIsNaN)
{
	rational zero(0);
	EXPECT_TRUE(zero.isNull());
	EXPECT_FALSE(zero.isNaN());

	rational nan = rational::NaN;
	EXPECT_TRUE(nan.isNaN());
	EXPECT_TRUE(nan.isNull());
}

TEST(CoreRational, NaNPropagation)
{
	rational a(1, 2);
	rational nan = rational::NaN;

	a += nan;
	EXPECT_TRUE(a.isNaN());
}

TEST(CoreRational, StreamOutput)
{
	std::ostringstream oss;
	oss << rational(3, 4);
	EXPECT_EQ(oss.str(), "3/4");
}

TEST(CoreRational, MinMaxConstants)
{
	EXPECT_TRUE(RATIONAL_MIN < rational(0));
	EXPECT_TRUE(RATIONAL_MAX > rational(0));
}
