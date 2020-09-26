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

#ifndef PROJECTSAVEASOTIOTASK_H
#define PROJECTSAVEASOTIOTASK_H

#include <opentimelineio/timeline.h>
#include <opentimelineio/track.h>

#include "project/item/sequence/sequence.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class ExportOTIOTask : public Task
{
  Q_OBJECT
public:
  ExportOTIOTask(ViewerOutput* sequence, const QString& filename);

protected:
  virtual bool Run() override;

private:
  opentimelineio::v1_0::Track* SerializeTrack(TrackOutput* track);

  bool SerializeTrackList(TrackList* list, opentimelineio::v1_0::Timeline *otio_timeline);

  ViewerOutput* sequence_;

  QString filename_;

};

OLIVE_NAMESPACE_EXIT

#endif // PROJECTSAVEASOTIOTASK_H
