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

#ifndef TIMELINEPOINTS_H
#define TIMELINEPOINTS_H

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "timelinemarker.h"
#include "timelineworkarea.h"

OLIVE_NAMESPACE_ENTER

class TimelinePoints
{
public:
  TimelinePoints() = default;

  TimelineMarkerList* markers();
  const TimelineMarkerList* markers() const;

  TimelineWorkArea* workarea();
  const TimelineWorkArea* workarea() const;

  void Load(QXmlStreamReader* reader);
  void Save(QXmlStreamWriter* writer) const;

private:
  TimelineMarkerList markers_;

  TimelineWorkArea workarea_;

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEPOINTS_H
