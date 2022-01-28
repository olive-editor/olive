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

#include "load.h"

#include <QApplication>

#include "node/project/serializer/serializer.h"

namespace olive {

ProjectLoadTask::ProjectLoadTask(const QString &filename) :
  ProjectLoadBaseTask(filename)
{
}

bool ProjectLoadTask::Run()
{
  project_ = new Project();

  project_->set_filename(GetFilename());

  ProjectSerializer::Result result = ProjectSerializer::Load(project_, GetFilename());

  switch (result.code()) {
  case ProjectSerializer::kSuccess:
    break;
  case ProjectSerializer::kProjectTooOld:
    SetError(tr("This project is from a version of Olive that is no longer supported in this version."));
    break;
  case ProjectSerializer::kProjectTooNew:
    SetError(tr("This project is from a newer version of Olive and cannot be opened in this version."));
    break;
  case ProjectSerializer::kUnknownVersion:
    SetError(tr("Failed to determine project version."));
    break;
  case ProjectSerializer::kFileError:
    SetError(tr("Failed to read file \"%1\" for reading.").arg(GetFilename()));
    break;
  case ProjectSerializer::kXmlError:
    SetError(tr("Failed to read XML document. File may be corrupt. Error was: %1").arg(result.GetDetails()));
    break;

    // Errors that should never be thrown by a load
  case ProjectSerializer::kOverwriteError:
    SetError(tr("Unknown error."));
    break;
  }

  if (result == ProjectSerializer::kSuccess) {
    project_->moveToThread(qApp->thread());
    return true;
  } else {
    delete project_;
    return false;
  }
}

}
