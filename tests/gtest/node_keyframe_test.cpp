#include <gtest/gtest.h>

#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "node/keyframe.h"
#include "node/value.h"

TEST(NodeKeyframe, SaveLoadRoundTrip)
{
	olive::NodeKeyframe key;
	key.set_input(QStringLiteral("Value"));
	key.set_time(olive::core::rational(1, 24));
	key.set_type(olive::NodeKeyframe::kLinear);
	key.set_value(42.0);
	key.set_bezier_control_in(QPointF(0.1, 0.2));
	key.set_bezier_control_out(QPointF(0.3, 0.4));

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("key"));
	key.save(&writer, olive::NodeValue::kFloat);
	writer.writeEndElement();
	writer.writeEndDocument();
	buffer.close();

	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	EXPECT_TRUE(reader.readNextStartElement());
	EXPECT_EQ(reader.name().toString(), QStringLiteral("key"));

	olive::NodeKeyframe loaded;
	EXPECT_TRUE(loaded.load(&reader, olive::NodeValue::kFloat));
	EXPECT_EQ(loaded.input(), QStringLiteral("Value"));
	EXPECT_EQ(loaded.time(), olive::core::rational(1, 24));
	EXPECT_EQ(loaded.type(), olive::NodeKeyframe::kLinear);
	EXPECT_DOUBLE_EQ(loaded.value().toDouble(), 42.0);
	EXPECT_DOUBLE_EQ(loaded.bezier_control_in().x(), 0.1);
	EXPECT_DOUBLE_EQ(loaded.bezier_control_in().y(), 0.2);
	EXPECT_DOUBLE_EQ(loaded.bezier_control_out().x(), 0.3);
	EXPECT_DOUBLE_EQ(loaded.bezier_control_out().y(), 0.4);
}

TEST(NodeKeyframe, TypeEnumeration)
{
	using olive::NodeKeyframe;

	EXPECT_NE(NodeKeyframe::kLinear, NodeKeyframe::kHold);
	EXPECT_NE(NodeKeyframe::kLinear, NodeKeyframe::kBezier);
}

TEST(NodeKeyframe, DefaultState)
{
	olive::NodeKeyframe key;
	EXPECT_TRUE(key.input().isEmpty());
	EXPECT_EQ(key.time(), olive::core::rational(0, 1));
	EXPECT_EQ(key.type(), olive::NodeKeyframe::kLinear);
	EXPECT_TRUE(key.value().isNull());
}

TEST(NodeKeyframe, SetValueRoundTrip)
{
	olive::NodeKeyframe key;

	key.set_value(QVariant::fromValue(olive::Color(0.1f, 0.2f, 0.3f, 1.0f)));
	const olive::Color c = key.value().value<olive::Color>();
	EXPECT_FLOAT_EQ(c.red(), 0.1f);
	EXPECT_FLOAT_EQ(c.green(), 0.2f);
	EXPECT_FLOAT_EQ(c.blue(), 0.3f);
}

TEST(NodeKeyframe, BezierControlDefaults)
{
	olive::NodeKeyframe key;
	EXPECT_DOUBLE_EQ(key.bezier_control_in().x(), 0.0);
	EXPECT_DOUBLE_EQ(key.bezier_control_in().y(), 0.0);
	EXPECT_DOUBLE_EQ(key.bezier_control_out().x(), 0.0);
	EXPECT_DOUBLE_EQ(key.bezier_control_out().y(), 0.0);
}
