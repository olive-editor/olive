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

#ifndef PROXYTASK_H
#define PROXYTASK_H

#include "project/item/footage/videostream.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class ProxyTask : public Task
{
public:
  ProxyTask(VideoStreamPtr stream, int divider);

protected:
  virtual void Action() override;

private:
  VideoStreamPtr stream_;

  int divider_;

};

OLIVE_NAMESPACE_EXIT

#endif // PROXYTASK_H
