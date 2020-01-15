#ifndef PROJECTLOADMANAGER_H
#define PROJECTLOADMANAGER_H

#include "projectfilemanagerbase.h"

class ProjectLoadManager : public ProjectFileManagerBase
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
