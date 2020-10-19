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

#ifndef PROJECTLOADMANAGER_H
#define PROJECTLOADMANAGER_H

#include "project/project.h"
#include "task/task.h"
#include "window/mainwindow/mainwindowlayoutinfo.h"

OLIVE_NAMESPACE_ENTER

class ProjectLoadTask : public Task
{
  Q_OBJECT
public:
  ProjectLoadTask(const QString& filename);

  ProjectPtr GetLoadedProject() const
  {
    return project_;
  }

  MainWindowLayoutInfo GetLoadedLayout() const
  {
    return layout_info_;
  }

  /**
   * @brief Returns the filename the project was saved as, but not necessarily where it is now
   *
   * May help for resolving relative paths.
   */
  const QString& GetFilenameProjectWasSavedAs() const
  {
    return project_saved_url_;
  }

protected:
  virtual bool Run() override;

private:
  ProjectPtr project_;

  MainWindowLayoutInfo layout_info_;

  QString project_saved_url_;

  QString filename_;

};

OLIVE_NAMESPACE_EXIT

#endif // PROJECTLOADMANAGER_H
