#include "timelinepoints.h"

#include "common/xmlutils.h"

TimelineMarkerList *TimelinePoints::markers()
{
  return &markers_;
}

const TimelineMarkerList *TimelinePoints::markers() const
{
  return &markers_;
}

const TimelineWorkArea *TimelinePoints::workarea() const
{
  return &workarea_;
}

void TimelinePoints::Load(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("markers")) {
      markers_.Load(reader);
    } else if (reader->name() == QStringLiteral("workarea")) {
      workarea_.Load(reader);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void TimelinePoints::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("points"));

  workarea_.Save(writer);

  markers_.Save(writer);

  writer->writeEndElement(); // points
}

TimelineWorkArea *TimelinePoints::workarea()
{
  return &workarea_;
}
