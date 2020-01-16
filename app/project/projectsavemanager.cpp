#include "projectsavemanager.h"

#include <QFile>
#include <QXmlStreamWriter>

ProjectSaveManager::ProjectSaveManager(Project *project) :
  project_(project)
{

}

void ProjectSaveManager::Action()
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
  }

  emit Succeeeded();
}
