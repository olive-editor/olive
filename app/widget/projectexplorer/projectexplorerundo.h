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

namespace olive {

/**
 * @brief An undo command for offlining footage when it is deleted from the project explorer
 */
class OfflineFootageCommand : public UndoCommand {
public:
  OfflineFootageCommand(const QVector<MediaInput*>& media)
  {
    foreach (MediaInput* i, media) {
      stream_data_.insert(i, i->stream());
    }

    project_ = media.first()->parent()->project();
  }

  virtual Project* GetRelevantProject() const override
  {
    return project_;
  }

  virtual void redo() override
  {
    for (auto it=stream_data_.cbegin(); it!=stream_data_.cend(); it++) {
      it.key()->SetStream(nullptr);
    }
  }

  virtual void undo() override
  {
    for (auto it=stream_data_.cbegin(); it!=stream_data_.cend(); it++) {
      it.key()->SetStream(it.value());
    }
  }

private:
  QMap<MediaInput*, Stream*> stream_data_;

  Project* project_;

};

}

#endif // PROJECTEXPLORERUNDO_H
