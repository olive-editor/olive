/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef CONFORMTASK_H
#define CONFORMTASK_H

#include "project/item/footage/footage.h"
#include "render/audioparams.h"
#include "task/task.h"

namespace olive {

class ConformTask : public Task
{
  Q_OBJECT
public:
  ConformTask(Footage* stream, int index, const AudioParams& params);

protected:
  virtual bool Run() override;

private:
  Footage* footage_;

  int index_;

  AudioParams params_;

};

}

#endif // CONFORMTASK_H
