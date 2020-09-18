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

#include <QDir>
#include <QFileInfo>

#include "core.h"
#include "codec/decoder.h"
#include "project/item/footage/footage.h"

OLIVE_NAMESPACE_ENTER

ProjectImportTask::ProjectImportTask(ProjectViewModel *model, Folder *folder, const QStringList &filenames) :
  command_(nullptr),
  model_(model),
  folder_(folder)
{
  foreach (const QString& f, filenames) {
    filenames_.append(f);
  }

  file_count_ = Core::CountFilesInFileList(filenames_);

  SetTitle(tr("Importing %1 files").arg(file_count_));
}

const int &ProjectImportTask::GetFileCount() const
{
  return file_count_;
}

bool ProjectImportTask::Run()
{
  command_ = new QUndoCommand();

  int imported = 0;

  Import(folder_, filenames_, imported, command_);

  if (IsCancelled()) {
    delete command_;
    command_ = nullptr;
    return false;
  } else {
    return true;
  }
}

void ProjectImportTask::Import(Folder *folder, const QFileInfoList &import, int &counter, QUndoCommand* parent_command)
{
  foreach (const QFileInfo& file_info, import) {
    if (IsCancelled()) {
      break;
    }

    // Check if this file is a directory
    if (file_info.isDir()) {

      // QDir::entryList only returns filenames, we can use entryInfoList() to get full paths
      QFileInfoList entry_list = QDir(file_info.absoluteFilePath()).entryInfoList();

      // Strip out "." and ".." (for some reason QDir::NoDotAndDotDot	doesn't work with entryInfoList, so we have to
      // check manually)
      for (int i=0;i<entry_list.size();i++) {
        if (entry_list.at(i).fileName() == "." || entry_list.at(i).fileName() == "..") {
          entry_list.removeAt(i);
          i--;
        }
      }

      // Only proceed if the empty actually has files in it
      if (!entry_list.isEmpty()) {
        // Create a folder corresponding to the directory

        ItemPtr f = std::make_shared<Folder>();

        f->set_name(file_info.fileName());

        // Create undoable command that adds the items to the model
        new ProjectViewModel::AddItemCommand(model_,
                                             folder,
                                             f,
                                             parent_command);

        // Recursively follow this path
        Import(static_cast<Folder*>(f.get()), entry_list, counter, parent_command);
      }

    } else {

      FootagePtr f = std::make_shared<Footage>();

      f->set_filename(file_info.absoluteFilePath());
      f->set_name(file_info.fileName());
      f->set_timestamp(file_info.lastModified());

      // Probe will fail if a project isn't set because ImageStream and its derivatives try to connect to the project's
      // ColorManager instance
      // FIXME: Perhaps re-think this approach at some point
      f->set_project(model_->project());

      Decoder::ProbeMedia(f.get(), &IsCancelled());

      f->set_project(nullptr);

      if (f->status() == Footage::kInvalid) {
        // Add to list so we can tell the user about it later
        invalid_files_.append(file_info.absoluteFilePath());
      } else {
        // Create undoable command that adds the items to the model
        new ProjectViewModel::AddItemCommand(model_,
                                             folder,
                                             f,
                                             parent_command);
      }

      counter++;

      emit ProgressChanged(static_cast<double>(counter) / static_cast<double>(file_count_));

    }
  }
}

OLIVE_NAMESPACE_EXIT
