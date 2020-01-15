#ifndef PROJECTSAVEMANAGER_H
#define PROJECTSAVEMANAGER_H

#include "projectfilemanagerbase.h"

class ProjectSaveManager : public ProjectFileManagerBase
{
  Q_OBJECT
public:
  ProjectSaveManager(Project* project);

protected:
  virtual void Action() override;

private:
  Project* project_;

};

#endif // PROJECTSAVEMANAGER_H
