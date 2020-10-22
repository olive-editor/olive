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

#include "projectexplorerundo.h"

OLIVE_NAMESPACE_ENTER

OfflineFootageCommand::OfflineFootageCommand(const QList<MediaInput *> &media, QUndoCommand* parent) :
  UndoCommand(parent)
{
  foreach (MediaInput* i, media) {
    stream_data_.insert(i, i->stream());
  }

  project_ = static_cast<Sequence*>(media.first()->parent())->project();
}

Project *OfflineFootageCommand::GetRelevantProject() const
{
  return project_;
}

void OfflineFootageCommand::redo_internal()
{
  for (auto it=stream_data_.cbegin(); it!=stream_data_.cend(); it++) {
    it.key()->SetStream(nullptr);
  }
}

void OfflineFootageCommand::undo_internal()
{
  for (auto it=stream_data_.cbegin(); it!=stream_data_.cend(); it++) {
    it.key()->SetStream(it.value());
  }
}

OLIVE_NAMESPACE_EXIT
