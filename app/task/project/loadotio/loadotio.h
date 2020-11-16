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

#ifndef OTIODECODER_H
#define OTIODECODER_H

#include "common/otioutils.h"
#include "project/project.h"
#include "task/project/load/loadbasetask.h"

OLIVE_NAMESPACE_ENTER

class LoadOTIOTask : public ProjectLoadBaseTask
{
  Q_OBJECT
public:
  LoadOTIOTask(const QString& filename);

protected:
  virtual bool Run() override;

};

OLIVE_NAMESPACE_EXIT

#endif // OTIODECODER_H
