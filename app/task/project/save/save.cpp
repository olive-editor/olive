/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "save.h"

#include <QDir>
#include <QFile>
#include <QXmlStreamWriter>

#include "common/filefunctions.h"
#include "core.h"
#include "node/project/serializer/serializer.h"

namespace olive {

ProjectSaveTask::ProjectSaveTask(Project *project) :
  project_(project)
{
  SetTitle(tr("Saving '%1'").arg(project->filename()));
}

bool ProjectSaveTask::Run()
{
  QString using_filename = override_filename_.isEmpty() ? project_->filename() : override_filename_;

  ProjectSerializer::SaveData data(project_, using_filename);

  ProjectSerializer::Result result = ProjectSerializer::Save(data);

  bool success = false;

  switch (result.code()) {
  case ProjectSerializer::kSuccess:
    success = true;
    break;
  case ProjectSerializer::kXmlError:
    SetError(tr("Failed to write XML data."));
    break;
  case ProjectSerializer::kFileError:
    SetError(tr("Failed to open file \"%1\" for writing.").arg(result.GetDetails()));
    break;
  case ProjectSerializer::kOverwriteError:
    SetError(tr("Failed to overwrite \"%1\". Project has been saved as \"%2\" instead.")
             .arg(using_filename, result.GetDetails()));
    success = true;
    break;

    // Errors that should never be thrown by a save
  case ProjectSerializer::kProjectTooNew:
  case ProjectSerializer::kProjectTooOld:
  case ProjectSerializer::kUnknownVersion:
    SetError(tr("Unknown error."));
    break;
  }

  return success;
}

}
