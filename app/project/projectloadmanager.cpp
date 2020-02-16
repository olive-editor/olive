#include "projectloadmanager.h"

#include <QApplication>
#include <QFile>
#include <QXmlStreamReader>

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

    while (!reader.atEnd()) {
      reader.readNext();

      if (reader.isStartElement()) {
        if (reader.name() == "version") {
          reader.readNext();

          qDebug() << "Project version:" << reader.text();
        } else if (reader.name() == "project") {
          ProjectPtr project = std::make_shared<Project>();

          project->set_filename(filename_);

          project->Load(&reader, &IsCancelled());

          // Ensure project is in main thread
          moveToThread(qApp->thread());

          if (!IsCancelled()) {
            emit ProjectLoaded(project);
          }
        }
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
