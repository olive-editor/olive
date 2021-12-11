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

#ifndef PROJECTSAVEASOTIOTASK_H
#define PROJECTSAVEASOTIOTASK_H

#ifdef USE_OTIO

#include <opentimelineio/timeline.h>
#include <opentimelineio/track.h>

#include "common/otioutils.h"
#include "node/project/project.h"
#include "task/task.h"

namespace olive {

class SaveOTIOTask : public Task
{
  Q_OBJECT
public:
  SaveOTIOTask(Project* project);

protected:
  virtual bool Run() override;

private:
  OTIO::Timeline* SerializeTimeline(Sequence* sequence);

  OTIO::Track* SerializeTrack(Track* track, double sequence_rate);

  bool SerializeTrackList(TrackList* list, OTIO::Timeline *otio_timeline, double sequence_rate);

  Project* project_;

};

}

#endif

#endif // PROJECTSAVEASOTIOTASK_H
