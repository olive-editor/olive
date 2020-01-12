#include "projectsavemanager.h"

#include <QFile>

ProjectSaveManager::ProjectSaveManager(Project *project) :
  project_(project),
  cancelled_(false)
{

}

void ProjectSaveManager::Start()
{
  QFile project_file(project_->filename());

  if (project_file.open(QFile::WriteOnly | QFile::Text)) {
    QXmlStreamWriter writer(&project_file);
    writer.setAutoFormatting(true);

    writer.writeStartDocument();

    writer.writeTextElement("version", "0.2.0");

    project_->Save(&writer);

    writer.writeEndDocument();

    project_file.close();
  }

  emit Finished();
}

void ProjectSaveManager::Cancel()
{
  cancelled_ = true;
}
