#include <gtest/gtest.h>

#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "timeline/timelinemarker.h"

TEST(TimelineMarker, SaveLoadRoundTrip)
{
	olive::TimelineMarker marker;
	marker.set_time(olive::core::rational(10, 1));
	marker.set_name(QStringLiteral("Marker"));
	marker.set_color(5);

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("marker"));
	marker.save(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();
	buffer.close();

	olive::TimelineMarker loaded;
	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	EXPECT_TRUE(reader.readNextStartElement());
	EXPECT_EQ(reader.name().toString(), QStringLiteral("marker"));
	loaded.load(&reader);

	EXPECT_EQ(loaded.time().in(), olive::core::rational(10, 1));
	EXPECT_EQ(loaded.name(), QStringLiteral("Marker"));
	EXPECT_EQ(loaded.color(), 5);
}

TEST(TimelineMarker, DefaultConstruction)
{
	olive::TimelineMarker marker;
	EXPECT_TRUE(marker.name().isEmpty());
	EXPECT_EQ(marker.time().in(), olive::core::rational(0, 1));
}

TEST(TimelineMarkerList, OrderAndLookup)
{
	olive::TimelineMarkerList list;
	olive::TimelineMarker marker_a(
		1,
		olive::core::TimeRange(olive::core::rational(10, 1),
							   olive::core::rational(10, 1)),
		QStringLiteral("A"), &list);
	olive::TimelineMarker marker_b(
		2,
		olive::core::TimeRange(olive::core::rational(5, 1),
							   olive::core::rational(5, 1)),
		QStringLiteral("B"), &list);
	olive::TimelineMarker marker_c(
		3,
		olive::core::TimeRange(olive::core::rational(20, 1),
							   olive::core::rational(20, 1)),
		QStringLiteral("C"), &list);

	ASSERT_EQ(list.size(), 3);
	auto it = list.cbegin();
	EXPECT_EQ((*it)->time().in(), olive::core::rational(5, 1));
	++it;
	EXPECT_EQ((*it)->time().in(), olive::core::rational(10, 1));
	++it;
	EXPECT_EQ((*it)->time().in(), olive::core::rational(20, 1));

	EXPECT_EQ(list.GetMarkerAtTime(olive::core::rational(10, 1)), &marker_a);
	EXPECT_EQ(list.GetClosestMarkerToTime(olive::core::rational(7, 1)),
			  &marker_b);
	EXPECT_EQ(list.GetClosestMarkerToTime(olive::core::rational(9, 1)),
			  &marker_a);
}

TEST(TimelineMarkerList, GetMarkerAtTimeReturnsNullWhenEmpty)
{
	olive::TimelineMarkerList list;
	EXPECT_EQ(list.GetMarkerAtTime(olive::core::rational(10, 1)), nullptr);
	EXPECT_EQ(list.GetClosestMarkerToTime(olive::core::rational(10, 1)),
			  nullptr);
}

TEST(TimelineMarkerList, SaveLoadWithUnknownElements)
{
	olive::TimelineMarkerList list;
	olive::TimelineMarker marker(
		4,
		olive::core::TimeRange(olive::core::rational(12, 1),
							   olive::core::rational(15, 1)),
		QStringLiteral("Span"), &list);

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("markers"));
	writer.writeStartElement(QStringLiteral("unknown"));
	writer.writeEndElement();
	list.save(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();
	buffer.close();

	olive::TimelineMarkerList loaded;
	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	EXPECT_TRUE(reader.readNextStartElement());
	EXPECT_EQ(reader.name().toString(), QStringLiteral("markers"));
	EXPECT_TRUE(loaded.load(&reader));
	EXPECT_EQ(loaded.size(), 1);
	EXPECT_EQ(loaded.front()->name(), QStringLiteral("Span"));
	EXPECT_EQ(loaded.front()->time().in(), olive::core::rational(12, 1));
}

TEST(TimelineMarkerCommands, AddRemoveAndChange)
{
	olive::TimelineMarkerList list;
	olive::MarkerAddCommand add(
		&list,
		olive::core::TimeRange(olive::core::rational(1, 1),
							   olive::core::rational(2, 1)),
		QStringLiteral("One"), 1);
	add.redo_now();
	ASSERT_EQ(list.size(), 1);
	auto *marker = list.front();
	EXPECT_EQ(marker->name(), QStringLiteral("One"));

	olive::MarkerChangeNameCommand rename(marker, QStringLiteral("Renamed"));
	rename.redo_now();
	EXPECT_EQ(marker->name(), QStringLiteral("Renamed"));
	rename.undo_now();
	EXPECT_EQ(marker->name(), QStringLiteral("One"));

	olive::MarkerChangeColorCommand recolor(marker, 9);
	recolor.redo_now();
	EXPECT_EQ(marker->color(), 9);
	recolor.undo_now();
	EXPECT_EQ(marker->color(), 1);

	olive::MarkerRemoveCommand remove(marker);
	remove.redo_now();
	EXPECT_TRUE(list.empty());
	remove.undo_now();
	EXPECT_EQ(list.size(), 1);

	olive::TimelineMarker other(
		2,
		olive::core::TimeRange(olive::core::rational(5, 1),
							   olive::core::rational(5, 1)),
		QStringLiteral("Two"), &list);
	EXPECT_EQ(list.front()->time().in(), olive::core::rational(1, 1));

	olive::MarkerChangeTimeCommand move(
		marker, olive::core::TimeRange(olive::core::rational(0, 1),
									   olive::core::rational(0, 1)));
	move.redo_now();
	EXPECT_EQ(list.front(), marker);
	move.undo_now();
	EXPECT_EQ(list.front()->time().in(), olive::core::rational(1, 1));
}

TEST(TimelineMarkerCommands, AddCommandUndo)
{
	olive::TimelineMarkerList list;
	olive::MarkerAddCommand add(
		&list,
		olive::core::TimeRange(olive::core::rational(5, 1),
							   olive::core::rational(5, 1)),
		QStringLiteral("UndoMe"), 2);
	add.redo_now();
	EXPECT_EQ(list.size(), 1);
	add.undo_now();
	EXPECT_TRUE(list.empty());
}
