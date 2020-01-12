#ifndef PROJECTLOADMANAGER_H
#define PROJECTLOADMANAGER_H

#include "projectfilemanagerbase.h"

class ProjectLoadManager : public ProjectFileManagerBase
{
  Q_OBJECT
public:
  ProjectLoadManager(const QString& filename);

public slots:
  /**
   * @brief Start the load process
   *
   * It's recommended to invoke this through Qt signals/slots/QueuedConnection after moving this object to a separate
   * thread.
   */
  virtual void Start() override;

signals:
  void ProjectLoaded(ProjectPtr project);

private:
  QString filename_;

};

#endif // PROJECTLOADMANAGER_H
