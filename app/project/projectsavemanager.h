#ifndef PROJECTSAVEMANAGER_H
#define PROJECTSAVEMANAGER_H

#include "project/project.h"
#include "task/task.h"

class ProjectSaveManager : public Task
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
