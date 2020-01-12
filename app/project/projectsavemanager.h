#ifndef PROJECTSAVEMANAGER_H
#define PROJECTSAVEMANAGER_H

#include <QObject>

#include "project.h"

class ProjectSaveManager : public QObject
{
  Q_OBJECT
public:
  ProjectSaveManager(Project* project);

public slots:
  void Start();

signals:
  void ProgressChanged(int);

  void Finished();

private:
  Project* project_;

};

#endif // PROJECTSAVEMANAGER_H
