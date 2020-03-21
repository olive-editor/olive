#include "projectloadmanager.h"

#include <QApplication>
#include <QFile>
#include <QXmlStreamReader>

#include "common/xmlutils.h"

ProjectLoadManager::ProjectLoadManager(const QString &filename) :
  filename_(filename)
{
  SetTitle(tr("Loading '%1'").arg(filename));
}

void ProjectLoadManager::Action()
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

            project->Load(&reader, &IsCancelled());

            // Ensure project is in main thread
            moveToThread(qApp->thread());

            if (!IsCancelled()) {
              emit ProjectLoaded(project);
            }
          } else {
            reader.skipCurrentElement();
          }
        }
      } else {
        reader.skipCurrentElement();
      }
    }

    if (reader.hasError()) {
      qDebug() << "Found XML error:" << reader.errorString();
      emit Failed(reader.errorString());
    } else {
      emit Succeeded();
    }

    project_file.close();
  }


}
