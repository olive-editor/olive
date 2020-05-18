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

#include "save.h"

#include <QFile>
#include <QXmlStreamWriter>

OLIVE_NAMESPACE_ENTER

ProjectSaveTask::ProjectSaveTask(ProjectPtr project) :
  project_(project)
{
  SetTitle(tr("Saving '%1'").arg(project->filename()));
}

bool ProjectSaveTask::Run()
{
  QFile project_file(project_->filename());

  if (project_file.open(QFile::WriteOnly | QFile::Text)) {
    QXmlStreamWriter writer(&project_file);
    writer.setAutoFormatting(true);

    writer.writeStartDocument();

    writer.writeStartElement("olive");

    writer.writeTextElement("version", "0.2.0");

    project_->Save(&writer);

    writer.writeEndElement(); // olive

    writer.writeEndDocument();

    project_file.close();

    return true;
  } else {
    SetError(tr("Failed to open file \"%1\" for writing.").arg(project_->filename()));
    return false;
  }
}

OLIVE_NAMESPACE_EXIT
