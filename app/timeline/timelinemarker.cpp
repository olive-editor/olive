#include "timelinemarker.h"

#include "common/xmlutils.h"

TimelineMarker::TimelineMarker(const TimeRange &time, const QString &name, QObject *parent) :
  QObject(parent),
  time_(time),
  name_(name)
{
}

const TimeRange &TimelineMarker::time() const
{
  return time_;
}

void TimelineMarker::set_time(const TimeRange &time)
{
  time_ = time;
  emit TimeChanged(time_);
}

const QString &TimelineMarker::name() const
{
  return name_;
}

void TimelineMarker::set_name(const QString &name)
{
  name_ = name;
  emit NameChanged(name_);
}

void TimelineMarkerList::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("markers"));

  foreach (TimelineMarker* marker, markers_) {
    writer->writeStartElement(QStringLiteral("marker"));

    writer->writeAttribute(QStringLiteral("name"), marker->name());

    writer->writeAttribute(QStringLiteral("in"), marker->time().in().toString());
    writer->writeAttribute(QStringLiteral("out"), marker->time().out().toString());

    writer->writeEndElement(); // marker
  }

  writer->writeEndElement(); // markers
}

TimelineMarkerList::~TimelineMarkerList()
{
  qDeleteAll(markers_);
}

void TimelineMarkerList::AddMarker(const TimeRange &time, const QString &name)
{
  TimelineMarker* m = new TimelineMarker(time, name);
  markers_.append(m);
  emit MarkerAdded(m);
}

void TimelineMarkerList::RemoveMarker(TimelineMarker *marker)
{
  for (int i=0;i<markers_.size();i++) {
    TimelineMarker* m = markers_.at(i);

    if (m == marker) {
      markers_.removeAt(i);
      emit MarkerRemoved(m);
      delete m;
      break;
    }
  }
}

const QList<TimelineMarker*> &TimelineMarkerList::list() const
{
  return markers_;
}

void TimelineMarkerList::Load(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("marker")) {
      QString name;
      TimeRange range;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("name")) {
          name = attr.value().toString();
        } else if (attr.name() == QStringLiteral("in")) {
          range.set_in(rational::fromString(attr.value().toString()));
        } else if (attr.name() == QStringLiteral("out")) {
          range.set_out(rational::fromString(attr.value().toString()));
        }
      }
    }

    reader->skipCurrentElement();
  }
}
