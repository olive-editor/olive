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

#ifndef PROJECTSAVEMANAGER_H
#define PROJECTSAVEMANAGER_H

#include "project/project.h"
#include "task/task.h"

namespace olive {

class ProjectSaveTask : public Task
{
  Q_OBJECT
public:
  ProjectSaveTask(ProjectPtr project);

  ProjectPtr GetProject() const
  {
    return project_;
  }

protected:
  virtual bool Run() override;

private:
  ProjectPtr project_;

};

}

#endif // PROJECTSAVEMANAGER_H
