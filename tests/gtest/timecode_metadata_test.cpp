#include <gtest/gtest.h>

#include <QDir>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "codec/timecodemetadata.h"
#include "node/project/footage/footage.h"
#include "node/project/footage/footagedescription.h"

TEST(TimecodeMetadata, ParsesNonDropFrameTimecode)
{
	const olive::TimecodeMetadata::SourceTime parsed =
		olive::TimecodeMetadata::FromTimecodeString(
			QStringLiteral("01:02:03:12"), olive::core::rational(1, 24));

	ASSERT_TRUE(parsed.valid);
	EXPECT_EQ(parsed.source, QStringLiteral("timecode"));
	EXPECT_EQ(parsed.time, olive::core::rational(1 * 3600 + 2 * 60 + 3, 1) +
							   olive::core::rational(12, 24));
}

TEST(TimecodeMetadata, ParsesDropFrameTimecode)
{
	const olive::TimecodeMetadata::SourceTime parsed =
		olive::TimecodeMetadata::FromTimecodeString(
			QStringLiteral("00:01:00;02"), olive::core::rational(1001, 30000));

	ASSERT_TRUE(parsed.valid);
	EXPECT_EQ(parsed.source, QStringLiteral("timecode"));
	EXPECT_GT(parsed.time, olive::core::rational(59));
	EXPECT_LT(parsed.time, olive::core::rational(61));
}

TEST(TimecodeMetadata, ParsesBwfTimeReference)
{
	const olive::TimecodeMetadata::SourceTime parsed =
		olive::TimecodeMetadata::FromBwfTimeReference(QStringLiteral("96000"),
													  48000);

	ASSERT_TRUE(parsed.valid);
	EXPECT_EQ(parsed.source, QStringLiteral("bwf_time_reference"));
	EXPECT_EQ(parsed.time, olive::core::rational(2));
}

TEST(TimecodeMetadata, ParsesLargeBwfTimeReferenceWithoutTruncation)
{
	const olive::TimecodeMetadata::SourceTime parsed =
		olive::TimecodeMetadata::FromBwfTimeReference(
			QStringLiteral("4294967296"), 48000);

	ASSERT_TRUE(parsed.valid);
	EXPECT_GT(parsed.time, olive::core::rational(89478));
	EXPECT_LT(parsed.time, olive::core::rational(89479));
}

TEST(TimecodeMetadata, RejectsInvalidMetadata)
{
	EXPECT_FALSE(olive::TimecodeMetadata::FromTimecodeString(
					 QString(), olive::core::rational(1, 24))
					 .valid);
	EXPECT_FALSE(olive::TimecodeMetadata::FromBwfTimeReference(
					 QStringLiteral("not-a-number"), 48000)
					 .valid);
	EXPECT_FALSE(
		olive::TimecodeMetadata::FromBwfTimeReference(QStringLiteral("123"), 0)
			.valid);
	EXPECT_FALSE(
		olive::TimecodeMetadata::FromTimecodeString(
			QStringLiteral("not-a-timecode"), olive::core::rational(1, 24))
			.valid);
}

TEST(TimecodeMetadata, FromBwfTimeReferenceZeroSampleRateIsInvalid)
{
	EXPECT_FALSE(
		olive::TimecodeMetadata::FromBwfTimeReference(QStringLiteral("0"), 0)
			.valid);
}

TEST(TimecodeMetadata, FootageDescriptionWithoutSourceStartTime)
{
	olive::FootageDescription desc(QStringLiteral("ffmpeg"));
	EXPECT_FALSE(desc.HasSourceStartTime());
}

TEST(TimecodeMetadata, FootageDescriptionCachesSourceStartTime)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path =
		QDir(dir.path()).filePath(QStringLiteral("footage-cache.xml"));

	olive::FootageDescription desc(QStringLiteral("ffmpeg"));
	desc.SetSourceStartTime(olive::core::rational(96000, 48000),
							QStringLiteral("bwf_time_reference"));
	ASSERT_TRUE(desc.Save(path));

	olive::FootageDescription loaded;
	ASSERT_TRUE(loaded.Load(path));
	ASSERT_TRUE(loaded.HasSourceStartTime());
	EXPECT_EQ(loaded.source_start_time(), olive::core::rational(2));
	EXPECT_EQ(loaded.source_start_time_source(),
			  QStringLiteral("bwf_time_reference"));
}

TEST(TimecodeMetadata, FootagePersistsSourceStartTime)
{
	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("custom"));
	writer.writeTextElement(QStringLiteral("timestamp"), QStringLiteral("0"));
	writer.writeStartElement(QStringLiteral("sourcestarttime"));
	writer.writeAttribute(QStringLiteral("source"), QStringLiteral("timecode"));
	writer.writeCharacters(QStringLiteral("3600/1"));
	writer.writeEndElement();
	writer.writeEndElement();
	writer.writeEndDocument();

	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("custom"));

	olive::Footage footage;
	ASSERT_TRUE(footage.LoadCustom(&reader, nullptr));
	ASSERT_TRUE(footage.HasSourceStartTime());
	EXPECT_EQ(footage.source_start_time(), olive::core::rational(3600));
	EXPECT_EQ(footage.source_start_time_source(), QStringLiteral("timecode"));
}

TEST(TimecodeMetadata, FootageClearSourceStartTime)
{
	olive::Footage footage;
	footage.SetSourceStartTime(olive::core::rational(3600),
							   QStringLiteral("manual"));
	ASSERT_TRUE(footage.HasSourceStartTime());

	footage.ClearSourceStartTime();

	EXPECT_FALSE(footage.HasSourceStartTime());
	EXPECT_EQ(footage.source_start_time(), olive::core::rational());
	EXPECT_TRUE(footage.source_start_time_source().isEmpty());
}

TEST(TimecodeMetadata, FootageSetSourceStartTimeOverridesPreviousValue)
{
	olive::Footage footage;
	footage.SetSourceStartTime(olive::core::rational(3600),
							   QStringLiteral("timecode"));

	// A manual edit replaces both the value and the recorded source
	footage.SetSourceStartTime(olive::core::rational(1800),
							   QStringLiteral("manual"));

	EXPECT_TRUE(footage.HasSourceStartTime());
	EXPECT_EQ(footage.source_start_time(), olive::core::rational(1800));
	EXPECT_EQ(footage.source_start_time_source(), QStringLiteral("manual"));
}
