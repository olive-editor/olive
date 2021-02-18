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

#include "load.h"

#include <QApplication>
#include <QFile>
#include <QXmlStreamReader>

#include "common/xmlutils.h"
#include "core.h"

namespace olive {

ProjectLoadTask::ProjectLoadTask(const QString &filename) :
  ProjectLoadBaseTask(filename)
{
}

bool ProjectLoadTask::Run()
{
  QFile project_file(GetFilename());

  if (project_file.open(QFile::ReadOnly | QFile::Text)) {
    QXmlStreamReader reader(&project_file);

    uint project_version;

    while (XMLReadNextStartElement(&reader)) {
      if (reader.name() == QStringLiteral("olive")) {
        while(XMLReadNextStartElement(&reader)) {
          if (reader.name() == QStringLiteral("version")) {
            bool ok;

            project_version = reader.readElementText().toUInt(&ok);

            if (!ok) {
              SetError(tr("Failed to determine project's version identifier."));
              return false;
            } else if (project_version > Core::kProjectVersion) {
              // Project is newer than we support
              SetError(tr("This project is newer than this version of Olive and cannot be opened."));
              return false;
            } else if (project_version < 210122) { // Change this if we drop support for a project version
              // Project is older than we support
              SetError(tr("This project is from a version of Olive that is no longer supported in this version."));
              return false;
            }
          } else if (reader.name() == QStringLiteral("url")) {
            project_saved_url_ = reader.readElementText();
          } else if (reader.name() == QStringLiteral("project")) {
            project_ = new Project();

            project_->set_filename(GetFilename());

            project_->Load(&reader, &layout_info_, project_version, &IsCancelled());

            // Ensure project is in main thread
            project_->moveToThread(qApp->thread());
            break;
          } else {
            reader.skipCurrentElement();
          }
        }
      } else if (reader.name() == QStringLiteral("project")) {
        // 0.1 projects use "project" as the root instead of Olive. We don't currently support
        // these projects
        SetError(tr("This project is from a version of Olive that is no longer supported in this version."));
        return false;
      } else {
        reader.skipCurrentElement();
      }
    }

    project_file.close();

    emit ProgressChanged(1);

    if (reader.hasError()) {
      SetError(reader.errorString());
      return false;
    } else {
      return true;
    }

  } else {
    SetError(tr("Failed to read file \"%1\" for reading.").arg(GetFilename()));
    return false;
  }
}

}
