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

#include "save.h"

#include <QDir>
#include <QFile>
#include <QXmlStreamWriter>

#include "common/filefunctions.h"
#include "core.h"

namespace olive {

ProjectSaveTask::ProjectSaveTask(Project *project) :
  project_(project)
{
  SetTitle(tr("Saving '%1'").arg(project->filename()));
}

bool ProjectSaveTask::Run()
{
  // File to temporarily save to (ensures we can't half-write the user's main file and crash)
  QString temp_save = FileFunctions::GetSafeTemporaryFilename(project_->filename());

  QFile project_file(temp_save);

  if (project_file.open(QFile::WriteOnly | QFile::Text)) {
    QXmlStreamWriter writer(&project_file);
    writer.setAutoFormatting(true);

    writer.writeStartDocument();

    writer.writeStartElement("olive");

    // Version is stored in YYMMDD from whenever the project format was last changed
    // Allows easy integer math for checking project versions.
    writer.writeTextElement(QStringLiteral("version"), QString::number(Core::kProjectVersion));

    writer.writeTextElement("url", project_->filename());

    writer.writeStartElement(QStringLiteral("project"));

    project_->Save(&writer);

    writer.writeEndElement(); // project

    writer.writeEndElement(); // olive

    writer.writeEndDocument();

    project_file.close();

    if (writer.hasError()) {
      SetError(tr("Failed to write XML data"));
      return false;
    }

    // Save was successful, we can now rewrite the original file
    if (FileFunctions::RenameFileAllowOverwrite(temp_save, project_->filename())) {
      return true;
    } else {
      SetError(tr("Failed to overwrite \"%1\". Project has been saved as \"%2\" instead.")
               .arg(project_->filename(), temp_save));
      return false;
    }
  } else {
    SetError(tr("Failed to open temporary file \"%1\" for writing.").arg(temp_save));
    return false;
  }
}

}
