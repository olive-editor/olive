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

#ifndef PROJECTEXPLORERUNDO_H
#define PROJECTEXPLORERUNDO_H

#include "node/input/media/media.h"
#include "undo/undocommand.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief An undo command for offlining footage when it is deleted from the project explorer
 */
class OfflineFootageCommand : public UndoCommand {
public:
  OfflineFootageCommand(const QList<MediaInput*>& media,  QUndoCommand* parent = nullptr);

  virtual Project* GetRelevantProject() const override;

protected:
  virtual void redo_internal() override;

  virtual void undo_internal() override;

private:
  QMap<MediaInput*, StreamPtr> stream_data_;

  Project* project_;

};

OLIVE_NAMESPACE_EXIT

#endif // PROJECTEXPLORERUNDO_H
