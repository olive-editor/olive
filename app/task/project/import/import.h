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

#ifndef PROJECTIMPORTMANAGER_H
#define PROJECTIMPORTMANAGER_H

#include <QFileInfoList>
#include <QUndoCommand>

#include "project/projectviewmodel.h"
#include "task/task.h"

OLIVE_NAMESPACE_ENTER

class ProjectImportTask : public Task
{
  Q_OBJECT
public:
  ProjectImportTask(ProjectViewModel* model, Folder* folder, const QStringList& filenames);

  const int& GetFileCount() const;

  QUndoCommand* GetCommand() const
  {
    return command_;
  }

protected:
  virtual bool Run() override;

private:
  void Import(Folder* folder, const QFileInfoList &import, int& counter, QUndoCommand *parent_command);

  QUndoCommand* command_;

  ProjectViewModel* model_;

  Folder* folder_;

  QFileInfoList filenames_;

  int file_count_;

};

OLIVE_NAMESPACE_EXIT

#endif // PROJECTIMPORTMANAGER_H
