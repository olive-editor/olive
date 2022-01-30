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

#ifndef MARKERCOPYPASTEWIDGET_H
#define MARKERCOPYPASTEWIDGET_H

#include <QWidget>
#include <QUndoCommand>

#include "widget/marker/marker.h"
#include "node/project/serializer/serializer.h"
#include "timeline/timelinemarker.h"
#include "undo/undocommand.h"

namespace olive {

class MarkerCopyPasteService
{
public:
  MarkerCopyPasteService() = default;

protected:
  void CopyMarkersToClipboard(const QVector<TimelineMarker*> &markers, void* userdata = nullptr);

  void PasteMarkersFromClipboard(TimelineMarkerList* list, MultiUndoCommand *command, rational offset);
};

}

#endif // MARKERCOPYPASTEWIDGET_H
