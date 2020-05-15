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

#ifndef CACHETASK_H
#define CACHETASK_H

#include "node/output/viewer/viewer.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class CacheTask : public Task
{
  Q_OBJECT
public:
  CacheTask(ViewerOutput* viewer, int divider, bool in_out_only);

protected:
  virtual void Action() override;

private:
  ViewerOutput* viewer_;

  bool in_out_only_;

  int divider_;

};

OLIVE_NAMESPACE_EXIT

#endif // CACHETASK_H
