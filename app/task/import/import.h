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

#ifndef IMPORT_H
#define IMPORT_H

#include "project/projectviewmodel.h"
#include "project/item/folder/folder.h"
#include "task/task.h"

/**
 * @brief The ImportTask class
 *
 * A background task to create Footage objects from a list of URLs, and then create ProbeTasks for each of them.
 *
 * Using this Task is the best way to import media into a project since it will run in the background/multithreaded
 * without pausing the main thread.
 */
class ImportTask : public Task
{
  Q_OBJECT
public:
  ImportTask(ProjectViewModel* model, Folder *parent, const QStringList& urls);

  virtual bool Action() override;

private:
  void Import(const QStringList& files, Folder* folder, QUndoCommand* parent_command);

  ProjectViewModel* model_;
  QStringList urls_;
  Folder* parent_;
};

#endif // IMPORT_H
