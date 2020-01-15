#ifndef PROJECTIMPORTMANAGER_H
#define PROJECTIMPORTMANAGER_H

#include <QFileInfoList>
#include <QUndoCommand>

#include "projectfilemanagerbase.h"
#include "projectviewmodel.h"

class ProjectImportManager : public ProjectFileManagerBase
{
  Q_OBJECT
public:
  ProjectImportManager(ProjectViewModel* model, Folder* folder, const QStringList& filenames);

  const int& GetFileCount();

protected:
  virtual void Action() override;

signals:
  void ImportComplete(QUndoCommand* command);

private:
  void Import(Folder* folder, const QFileInfoList &import, int& counter, QUndoCommand *parent_command);

  ProjectViewModel* model_;

  Folder* folder_;

  QFileInfoList filenames_;

  int file_count_;

};

#endif // PROJECTIMPORTMANAGER_H
