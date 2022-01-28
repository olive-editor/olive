/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "config/config.h"
#include "core.h"
#include "widget/marker/markerundo.h"

namespace olive {

TimelineMarker::TimelineMarker(const TimeRange &time, const QString &name, QObject *parent) :
  QObject(parent),
  time_(time),
  name_(name)
{
  color_ = Config::Current()[QStringLiteral("MarkerColor")].toInt();
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

void TimelineMarker::set_time_undo(TimeRange time) {
  UndoCommand *command = new MarkerChangeTimeCommand(Core::instance()->GetActiveProject(), this, time);

  Core::instance()->undo_stack()->push(command);
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

void TimelineMarker::set_name_undo(QString name)
{
  UndoCommand *command = new MarkerChangeNameCommand(Core::instance()->GetActiveProject(), this, name);

  Core::instance()->undo_stack()->push(command);
}

int TimelineMarker::color()
{
  return color_;
}

void TimelineMarker::set_color(int c)
{
  color_ = c;

  emit ColorChanged(color_);
}

bool TimelineMarker::active()
{
  return active_;
}

void TimelineMarker::set_active(bool active)
{
    active_ = active;

    emit ActiveChanged(active_);
}

void TimelineMarker::Save(QXmlStreamWriter* writer) const
{
  writer->writeStartElement(QStringLiteral("marker"));

  writer->writeAttribute(QStringLiteral("name"), name_);

  writer->writeAttribute(QStringLiteral("color"), QString::number(color_));

  writer->writeAttribute(QStringLiteral("in"), time_.in().toString());
  writer->writeAttribute(QStringLiteral("out"), time_.out().toString());

  writer->writeEndElement();  // marker
}

void TimelineMarkerList::Save(QXmlStreamWriter *writer) const
{
  foreach (TimelineMarker* marker, markers_) {
    marker->Save(writer);
  }
}
  
TimelineMarkerList::~TimelineMarkerList()
{
  qDeleteAll(markers_);
}

TimelineMarker* TimelineMarkerList::AddMarker(const TimeRange &time, const QString &name, int color)
{
  TimelineMarker* m = new TimelineMarker(time, name);
  if (color >= 0) {
    m->set_color(color);
  }
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
      int color = -1;
      rational in, out;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("name")) {
          name = attr.value().toString();
        }else if(attr.name() == QStringLiteral("color")){
          color = attr.value().toInt();
        } else if (attr.name() == QStringLiteral("in")) {
          in = rational::fromString(attr.value().toString());
        } else if (attr.name() == QStringLiteral("out")) {
          out = rational::fromString(attr.value().toString());
        }
      }

      AddMarker(TimeRange(in, out), name, color);
    }

    reader->skipCurrentElement();
  }
}

}
