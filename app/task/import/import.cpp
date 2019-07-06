
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

#include "import.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

// FIXME: Only used for test code
#include "panel/panelfocusmanager.h"
#include "panel/project/project.h"
// End test code

#include "project/item/footage/footage.h"
#include "task/probe/probe.h"
#include "task/taskmanager.h"
#include "undo/undostack.h"

ImportTask::ImportTask(ProjectViewModel *model, Folder *parent, const QStringList &urls) :
  model_(model),
  urls_(urls),
  parent_(parent)
{
  set_text(tr("Importing %1 files").arg(urls.size()));
}

bool ImportTask::Action()
{
  QUndoCommand* command = new QUndoCommand();

  Import(urls_, parent_, command);

  olive::undo_stack.push(command);

  return true;
}

void ImportTask::Import(const QStringList &files, Folder *folder, QUndoCommand *parent_command)
{
  for (int i=0;i<files.size();i++) {

    const QString& url = files.at(i);

    QFileInfo file_info(url);

    // Check if this file is a diretory
    if (file_info.isDir()) {

      // Use QDir to get a list of all the files in the directory
      QDir dir(url);

      // QDir::entryList only returns filenames, we can use entryInfoList() to get full paths
      QFileInfoList entry_list = dir.entryInfoList();

      // Only proceed if the empty actually has files in it
      if (!entry_list.isEmpty()) {
        // Create a folder corresponding to the directory
        Folder* f = new Folder();

        f->set_name(file_info.fileName());

        // Create undoable command that adds the items to the model
        new ProjectViewModel::AddItemCommand(model_,
                                             folder,
                                             f,
                                             parent_command);

        // Convert QFileInfoList into QStringList
        QStringList full_urls;

        foreach (QFileInfo info, entry_list) {
          if (info.fileName() != ".." && info.fileName() != ".") {
            full_urls.append(info.absoluteFilePath());
          }
        }

        // Recursively follow this path
        Import(full_urls, f, parent_command);
      }

    } else {

      Footage* f = new Footage();

      // FIXME: Is it possible for a file to go missing between the Import dialog and here?
      //        And what is the behavior/result of that?

      f->set_filename(url);
      f->set_name(file_info.fileName());
      f->set_timestamp(file_info.lastModified());

      // Create undoable command that adds the items to the model
      new ProjectViewModel::AddItemCommand(model_,
                                           folder,
                                           f,
                                           parent_command);

      // Create ProbeTask to analyze this media
      ProbeTask* pt = new ProbeTask(f);

      // The task won't work unless it's in the main thread and we're definitely not
      // FIXME: Should Tasks check what thread they're in and move themselves to the main thread?
      pt->moveToThread(qApp->thread());

      // Queue task in task manager
      olive::task_manager.AddTask(pt);

    }

    emit ProgressChanged(i * 100 / files.size());

  }
}
