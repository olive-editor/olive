/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timelinemarker.h"

#include "common/xmlutils.h"

OLIVE_NAMESPACE_ENTER

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
  foreach (TimelineMarker* marker, markers_) {
    writer->writeStartElement(QStringLiteral("marker"));

    writer->writeAttribute(QStringLiteral("name"), marker->name());

    writer->writeAttribute(QStringLiteral("in"), marker->time().in().toString());
    writer->writeAttribute(QStringLiteral("out"), marker->time().out().toString());

    writer->writeEndElement(); // marker
  }
}

TimelineMarkerList::~TimelineMarkerList()
{
  qDeleteAll(markers_);
}

TimelineMarker* TimelineMarkerList::AddMarker(const TimeRange &time, const QString &name)
{
  TimelineMarker* m = new TimelineMarker(time, name);
  markers_.append(m);
  emit MarkerAdded(m);
  return m;
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

OLIVE_NAMESPACE_EXIT
