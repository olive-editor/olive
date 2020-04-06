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

#include "timelinepoints.h"

#include "common/xmlutils.h"

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT
