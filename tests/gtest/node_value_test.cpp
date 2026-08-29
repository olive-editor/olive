#include <gtest/gtest.h>

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "node/value.h"

TEST(NodeValue, VectorRoundTrip)
{
	QVector2D v2(1.5f, -2.0f);
	QString encoded = olive::NodeValue::ValueToString(
		olive::NodeValue::kVec2, QVariant::fromValue(v2), false);
	QVariant decoded = olive::NodeValue::StringToValue(olive::NodeValue::kVec2,
													   encoded, false);
	QVector2D v2_out = decoded.value<QVector2D>();
	EXPECT_FLOAT_EQ(v2_out.x(), v2.x());
	EXPECT_FLOAT_EQ(v2_out.y(), v2.y());

	QVector3D v3(1.0f, 2.0f, 3.0f);
	encoded = olive::NodeValue::ValueToString(olive::NodeValue::kVec3,
											  QVariant::fromValue(v3), false);
	decoded = olive::NodeValue::StringToValue(olive::NodeValue::kVec3, encoded,
											  false);
	QVector3D v3_out = decoded.value<QVector3D>();
	EXPECT_FLOAT_EQ(v3_out.x(), v3.x());
	EXPECT_FLOAT_EQ(v3_out.y(), v3.y());
	EXPECT_FLOAT_EQ(v3_out.z(), v3.z());

	QVector4D v4(1.0f, 2.0f, 3.0f, 4.0f);
	encoded = olive::NodeValue::ValueToString(olive::NodeValue::kVec4,
											  QVariant::fromValue(v4), false);
	decoded = olive::NodeValue::StringToValue(olive::NodeValue::kVec4, encoded,
											  false);
	QVector4D v4_out = decoded.value<QVector4D>();
	EXPECT_FLOAT_EQ(v4_out.x(), v4.x());
	EXPECT_FLOAT_EQ(v4_out.y(), v4.y());
	EXPECT_FLOAT_EQ(v4_out.z(), v4.z());
	EXPECT_FLOAT_EQ(v4_out.w(), v4.w());
}

TEST(NodeValue, BinaryRoundTrip)
{
	QByteArray data("OliveTest");
	QString encoded =
		olive::NodeValue::ValueToString(olive::NodeValue::kBinary, data, false);
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kBinary, encoded, false);
	EXPECT_EQ(decoded.toByteArray(), data);
}

TEST(NodeValue, TypeClassification)
{
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kFloat));
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kColor));
	EXPECT_FALSE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kInt));

	EXPECT_TRUE(olive::NodeValue::type_is_numeric(olive::NodeValue::kInt));
	EXPECT_TRUE(olive::NodeValue::type_is_numeric(olive::NodeValue::kFloat));
	EXPECT_FALSE(olive::NodeValue::type_is_numeric(olive::NodeValue::kText));

	EXPECT_TRUE(olive::NodeValue::type_is_vector(olive::NodeValue::kVec2));
	EXPECT_TRUE(olive::NodeValue::type_is_vector(olive::NodeValue::kVec3));
	EXPECT_FALSE(olive::NodeValue::type_is_vector(olive::NodeValue::kFloat));

	EXPECT_TRUE(olive::NodeValue::type_is_buffer(olive::NodeValue::kTexture));
	EXPECT_TRUE(olive::NodeValue::type_is_buffer(olive::NodeValue::kSamples));
	EXPECT_FALSE(olive::NodeValue::type_is_buffer(olive::NodeValue::kColor));
}

TEST(NodeValue, DataTypeNameRoundTrip)
{
	for (int i = olive::NodeValue::kNone; i < olive::NodeValue::kDataTypeCount;
		 ++i) {
		auto type = static_cast<olive::NodeValue::Type>(i);
		QString name = olive::NodeValue::GetDataTypeName(type);
		if (name.isEmpty()) {
			continue;
		}
		EXPECT_EQ(olive::NodeValue::GetDataTypeFromName(name), type)
			<< name.toStdString();
	}
}

TEST(NodeValue, ConstructionAndAccessors)
{
	olive::NodeValue val(olive::NodeValue::kInt, static_cast<int64_t>(42));
	EXPECT_EQ(val.type(), olive::NodeValue::kInt);
	EXPECT_EQ(val.toInt(), 42);
	EXPECT_TRUE(val);

	val.set_tag(QStringLiteral("tag"));
	EXPECT_EQ(val.tag(), QStringLiteral("tag"));
}

TEST(NodeValueTable, PushAndGet)
{
	olive::NodeValueTable table;
	olive::NodeValue v(olive::NodeValue::kFloat, 3.14);
	table.Push(v);

	EXPECT_EQ(table.Count(), 1);
	EXPECT_FALSE(table.isEmpty());
	EXPECT_TRUE(table.Has(olive::NodeValue::kFloat));

	olive::NodeValue got = table.Get(olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(got.toDouble(), 3.14);
}

TEST(NodeValueTable, TakeRemovesValue)
{
	olive::NodeValueTable table;
	table.Push(
		olive::NodeValue(olive::NodeValue::kInt, static_cast<int64_t>(1)));
	table.Push(
		olive::NodeValue(olive::NodeValue::kInt, static_cast<int64_t>(2)));

	olive::NodeValue taken = table.Take(olive::NodeValue::kInt);
	EXPECT_EQ(taken.toInt(), 2);
	EXPECT_EQ(table.Count(), 1);
}

TEST(NodeValueTable, ClearEmptiesTable)
{
	olive::NodeValueTable table;
	table.Push(
		olive::NodeValue(olive::NodeValue::kText, QStringLiteral("hello")));
	table.Clear();
	EXPECT_TRUE(table.isEmpty());
	EXPECT_EQ(table.Count(), 0);
}
