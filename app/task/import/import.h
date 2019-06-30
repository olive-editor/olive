#ifndef IMPORT_H
#define IMPORT_H

#include "project/item/folder/folder.h"
#include "task/task.h"

class ImportTask : public Task
{
  Q_OBJECT
public:
  ImportTask(Folder *parent_, const QStringList& urls);

  virtual bool Action() override;

private:
  QStringList urls_;
  Folder* parent_;
};

#endif // IMPORT_H
