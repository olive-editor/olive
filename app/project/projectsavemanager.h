#ifndef PROJECTSAVEMANAGER_H
#define PROJECTSAVEMANAGER_H

#include <QObject>
#include <QXmlStreamWriter>

#include "project.h"

class ProjectSaveManager : public QObject
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
  void Start();

  /**
   * @brief Cancel the current save
   *
   * Always connect to this with a DirectConnection. Otherwise, it'll be queued AFTER the save function is already
   * complete.
   */
  void Cancel();

signals:
  void ProgressChanged(int);

  void Finished();

private:
  Project* project_;

  QAtomicInt cancelled_;

};

#endif // PROJECTSAVEMANAGER_H
