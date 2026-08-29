#include <gtest/gtest.h>

#include "olive/core/util/bezier.h"

using namespace olive::core;

TEST(CoreBezier, DefaultConstruction)
{
	Bezier b;
	EXPECT_DOUBLE_EQ(b.x(), 0.0);
	EXPECT_DOUBLE_EQ(b.y(), 0.0);
	EXPECT_DOUBLE_EQ(b.cp1_x(), 0.0);
	EXPECT_DOUBLE_EQ(b.cp1_y(), 0.0);
	EXPECT_DOUBLE_EQ(b.cp2_x(), 0.0);
	EXPECT_DOUBLE_EQ(b.cp2_y(), 0.0);
}

TEST(CoreBezier, ValueConstruction)
{
	Bezier b(1.0, 2.0);
	EXPECT_DOUBLE_EQ(b.x(), 1.0);
	EXPECT_DOUBLE_EQ(b.y(), 2.0);
}

TEST(CoreBezier, FullConstruction)
{
	Bezier b(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
	EXPECT_DOUBLE_EQ(b.x(), 1.0);
	EXPECT_DOUBLE_EQ(b.y(), 2.0);
	EXPECT_DOUBLE_EQ(b.cp1_x(), 3.0);
	EXPECT_DOUBLE_EQ(b.cp1_y(), 4.0);
	EXPECT_DOUBLE_EQ(b.cp2_x(), 5.0);
	EXPECT_DOUBLE_EQ(b.cp2_y(), 6.0);
}

TEST(CoreBezier, Setters)
{
	Bezier b;
	b.set_x(10.0);
	b.set_y(20.0);
	b.set_cp1_x(30.0);
	b.set_cp1_y(40.0);
	b.set_cp2_x(50.0);
	b.set_cp2_y(60.0);

	EXPECT_DOUBLE_EQ(b.x(), 10.0);
	EXPECT_DOUBLE_EQ(b.y(), 20.0);
	EXPECT_DOUBLE_EQ(b.cp1_x(), 30.0);
	EXPECT_DOUBLE_EQ(b.cp1_y(), 40.0);
	EXPECT_DOUBLE_EQ(b.cp2_x(), 50.0);
	EXPECT_DOUBLE_EQ(b.cp2_y(), 60.0);
}

TEST(CoreBezier, QuadraticXtoT)
{
	double t = Bezier::QuadraticXtoT(0.5, 0.0, 0.5, 1.0);
	EXPECT_NEAR(t, 0.5, 0.00001);

	t = Bezier::QuadraticXtoT(0.0, 0.0, 0.5, 1.0);
	EXPECT_NEAR(t, 0.0, 0.00001);

	t = Bezier::QuadraticXtoT(1.0, 0.0, 0.5, 1.0);
	EXPECT_NEAR(t, 1.0, 0.00001);
}

TEST(CoreBezier, QuadraticTtoY)
{
	EXPECT_NEAR(Bezier::QuadraticTtoY(0.0, 0.5, 1.0, 0.0), 0.0, 0.00001);
	EXPECT_NEAR(Bezier::QuadraticTtoY(0.0, 0.5, 1.0, 0.5), 0.5, 0.00001);
	EXPECT_NEAR(Bezier::QuadraticTtoY(0.0, 0.5, 1.0, 1.0), 1.0, 0.00001);
}

TEST(CoreBezier, QuadraticXtoY)
{
	Imath::V2d a(0.0, 0.0);
	Imath::V2d b(0.5, 0.5);
	Imath::V2d c(1.0, 1.0);

	EXPECT_NEAR(Bezier::QuadraticXtoY(0.5, a, b, c), 0.5, 0.00001);
}

TEST(CoreBezier, CubicXtoT)
{
	double t = Bezier::CubicXtoT(0.5, 0.0, 0.33, 0.66, 1.0);
	EXPECT_NEAR(t, 0.5, 0.01);
}

TEST(CoreBezier, CubicTtoY)
{
	EXPECT_NEAR(Bezier::CubicTtoY(0.0, 0.33, 0.66, 1.0, 0.0), 0.0, 0.00001);
	EXPECT_NEAR(Bezier::CubicTtoY(0.0, 0.33, 0.66, 1.0, 1.0), 1.0, 0.00001);
}

TEST(CoreBezier, CubicXtoY)
{
	Imath::V2d a(0.0, 0.0);
	Imath::V2d b(0.33, 0.0);
	Imath::V2d c(0.66, 1.0);
	Imath::V2d d(1.0, 1.0);

	double y = Bezier::CubicXtoY(0.5, a, b, c, d);
	EXPECT_GE(y, 0.0);
	EXPECT_LE(y, 1.0);
}

TEST(CoreBezier, VectorConverters)
{
	Bezier b(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
	Imath::V2d v = b.to_vec();
	EXPECT_DOUBLE_EQ(v.x, 1.0);
	EXPECT_DOUBLE_EQ(v.y, 2.0);

	Imath::V2d cp1 = b.control_point_1_to_vec();
	EXPECT_DOUBLE_EQ(cp1.x, 3.0);
	EXPECT_DOUBLE_EQ(cp1.y, 4.0);

	Imath::V2d cp2 = b.control_point_2_to_vec();
	EXPECT_DOUBLE_EQ(cp2.x, 5.0);
	EXPECT_DOUBLE_EQ(cp2.y, 6.0);
}
