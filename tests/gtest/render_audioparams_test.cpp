#include <gtest/gtest.h>

#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "node/project/serializer/typeserializer.h"
#include "olive/core/render/audioparams.h"

TEST(RenderAudioParams, SaveLoadRoundTrip)
{
	olive::AudioParams params;
	params.set_sample_rate(48000);
	params.set_enabled(true);
	params.set_time_base(olive::core::rational(1, 48000));

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("audioparams"));
	olive::TypeSerializer::SaveAudioParams(&writer, params);
	writer.writeEndElement();
	writer.writeEndDocument();
	buffer.close();

	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	EXPECT_TRUE(reader.readNextStartElement());
	EXPECT_EQ(reader.name().toString(), QStringLiteral("audioparams"));
	olive::AudioParams loaded = olive::TypeSerializer::LoadAudioParams(&reader);

	EXPECT_EQ(loaded.sample_rate(), 48000);
	EXPECT_TRUE(loaded.enabled());
	EXPECT_EQ(loaded.time_base(), olive::core::rational(1, 48000));
}
