#ifndef PROJECTSAVEMANAGER_H
#define PROJECTSAVEMANAGER_H

#include "projectfilemanagerbase.h"

class ProjectSaveManager : public ProjectFileManagerBase
{
  Q_OBJECT
public:
  ProjectSaveManager(Project* project);

public slots:
  /**
   * @brief Start the save process
   *
   * It's recommended to invoke this through Qt signals/slots/QueuedConnection after moving this object to a separate
   * thread.
   */
  virtual void Start() override;

private:
  Project* project_;

};

#endif // PROJECTSAVEMANAGER_H
