/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "codec/decoder.h"
#include "task/task.h"
#include "widget/projectexplorer/projectviewmodel.h"

namespace olive {

class ProjectImportTask : public Task
{
  Q_OBJECT
public:
  ProjectImportTask(Folder* folder, const QStringList& filenames);

  const int& GetFileCount() const;

  MultiUndoCommand* GetCommand() const
  {
    return command_;
  }

  const QStringList& GetInvalidFiles() const
  {
    return invalid_files_;
  }

  bool HasInvalidFiles() const
  {
    return !invalid_files_.isEmpty();
  }

  const QVector<Footage*> &GetImportedFootage() const { return imported_footage_; }

protected:
  virtual bool Run() override;

private:
  void Import(Folder* folder, QFileInfoList import, int& counter, MultiUndoCommand *parent_command);

  void ValidateImageSequence(Footage *footage, QFileInfoList &info_list, int index);

  void AddItemToFolder(Folder* folder, Node* item, MultiUndoCommand* command);

  static bool ItemIsStillImageFootageOnly(Footage *footage);

  static bool CompareStillImageSize(Footage *footage, const QSize& sz);

  static int64_t GetImageSequenceLimit(const QString &start_fn, int64_t start, bool up);

  MultiUndoCommand* command_;

  Folder* folder_;

  QFileInfoList filenames_;

  int file_count_;

  QStringList invalid_files_;

  QList<QString> image_sequence_ignore_files_;

  QVector<Footage*> imported_footage_;

};

}

#endif // PROJECTIMPORTMANAGER_H
