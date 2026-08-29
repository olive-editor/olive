#include <gtest/gtest.h>

#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/xmlutils.h"

TEST(CommonXmlUtils, ReadNextStartElement)
{
	QByteArray xml = "<root><child>value</child></root>";
	QBuffer buffer(&xml);
	buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&buffer);

	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("root"));
	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("child"));
}

TEST(CommonXmlUtils, ReadNextStartElementSkipsWhitespace)
{
	QByteArray xml = "\n  \n<root>\n\n<child/>\n</root>";
	QBuffer buffer(&xml);
	buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&buffer);

	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("root"));
	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("child"));
}

TEST(CommonXmlUtils, ReadNextStartElementReturnsFalseAtEnd)
{
	QByteArray xml = "<root/>";
	QBuffer buffer(&xml);
	buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&buffer);

	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("root"));
	EXPECT_FALSE(olive::XMLReadNextStartElement(&reader));
}

TEST(CommonXmlUtils, ReadNextStartElementSkipsUnknown)
{
	QByteArray xml = "<root><unknown/><known/></root>";
	QBuffer buffer(&xml);
	buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&buffer);

	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("unknown"));
	reader.skipCurrentElement();
	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("known"));
}

TEST(CommonXmlUtils, ReadNextStartElementWithCancel)
{
	QByteArray xml = "<root><child/></root>";
	QBuffer buffer(&xml);
	buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&buffer);

	olive::CancelAtom atom;
	EXPECT_TRUE(olive::XMLReadNextStartElement(&reader, &atom));
	EXPECT_EQ(reader.name().toString(), QStringLiteral("root"));
}

TEST(CommonXmlUtils, ReadNextStartElementRespectsCancel)
{
	QByteArray xml = "<root><child/></root>";
	QBuffer buffer(&xml);
	buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&buffer);

	olive::CancelAtom atom;
	atom.Cancel();
	EXPECT_FALSE(olive::XMLReadNextStartElement(&reader, &atom));
}
