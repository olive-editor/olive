#include "projectloadmanager.h"

#include <QFile>
#include <QXmlStreamReader>

ProjectLoadManager::ProjectLoadManager(const QString &filename) :
  filename_(filename)
{
}

void ProjectLoadManager::Start()
{
  QFile project_file(filename_);

  if (project_file.open(QFile::ReadOnly | QFile::Text)) {
    QXmlStreamReader reader(&project_file);

    while (!reader.atEnd()) {
      reader.readNext();

      if (reader.isStartElement()) {
        if (reader.name() == "version") {
          reader.readNext();

          qDebug() << "Project version:" << reader.text();
        } else if (reader.name() == "project") {
          ProjectPtr project = std::make_shared<Project>();

          project->set_filename(filename_);

          project->Load(&reader);

          emit ProjectLoaded(project);
        }
      }
    }

    if (reader.hasError()) {
      qDebug() << "Found XML error:" << reader.errorString();
    }

    project_file.close();
  }

  emit Finished();
}
