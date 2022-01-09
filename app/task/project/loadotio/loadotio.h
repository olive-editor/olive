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

#ifndef OTIODECODER_H
#define OTIODECODER_H

#ifdef USE_OTIO

#include "common/otioutils.h"
#include "node/project/project.h"
#include "task/project/load/loadbasetask.h"
#include <opentimelineio/serializableCollection.h>
#include <opentimelineio/composable.h>
#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"

namespace olive {

class LoadOTIOTask : public ProjectLoadBaseTask
{
  Q_OBJECT
public:
  LoadOTIOTask(const QString& filename);

protected:
  virtual bool Run() override;

private:
  ClipBlock* LoadClip(OTIO::Composable* otio_block, Track* track, Sequence* sequence, Folder* sequence_footage);
  
  GapBlock* LoadGap(OTIO::Composable* otio_block, Track* track, Sequence* sequence);

  CrossDissolveTransition* LoadTransition(OTIO::Composable* otio_block, Track* track, Sequence* sequence);

  Block* previous_block_;
  bool prev_block_transition_;
  bool transition_flag_;

  QMap<QString, Footage*> imported_footage_;
};

}

#endif

#endif // OTIODECODER_H
