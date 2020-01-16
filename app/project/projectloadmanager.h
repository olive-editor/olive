#ifndef PROJECTLOADMANAGER_H
#define PROJECTLOADMANAGER_H

#include "project/project.h"
#include "task/task.h"

class ProjectLoadManager : public Task
{
  Q_OBJECT
public:
  ProjectLoadManager(const QString& filename);

protected:
  virtual void Action() override;

signals:
  void ProjectLoaded(ProjectPtr project);

private:
  QString filename_;

};

#endif // PROJECTLOADMANAGER_H
