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

#include "load.h"

#include <QApplication>
#include <QFile>
#include <QXmlStreamReader>

#include "common/xmlutils.h"

OLIVE_NAMESPACE_ENTER

ProjectLoadTask::ProjectLoadTask(const QString &filename) :
  filename_(filename)
{
  SetTitle(tr("Loading '%1'").arg(filename));
}

bool ProjectLoadTask::Run()
{
  QFile project_file(filename_);

  if (project_file.open(QFile::ReadOnly | QFile::Text)) {
    QXmlStreamReader reader(&project_file);

    while (XMLReadNextStartElement(&reader)) {
      if (reader.name() == QStringLiteral("olive")) {
        while(XMLReadNextStartElement(&reader)) {
          if (reader.name() == QStringLiteral("version")) {
            qDebug() << "Project version:" << reader.readElementText();
          } else if (reader.name() == QStringLiteral("project")) {
            ProjectPtr project = std::make_shared<Project>();

            project->set_filename(filename_);

            MainWindowLayoutInfo layout;

            project->Load(&reader, &layout, &IsCancelled());

            // Ensure project is in main thread
            moveToThread(qApp->thread());

            if (!IsCancelled()) {
              projects_.append(project);
              layout_info_.append(layout);
            }
          } else {
            reader.skipCurrentElement();
          }
        }
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
    SetError(tr("Failed to read file \"%1\" for reading.").arg(filename_));
    return false;
  }
}

OLIVE_NAMESPACE_EXIT
