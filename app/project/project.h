#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <memory>

#include "project/item/folder/folder.h"

class Project : public QObject
{
  Q_OBJECT
public:
  Project();

  Folder* root();

  const QString& name();
  void set_name(const QString& s);

private:
  Folder root_;

  QString name_;
};

using ProjectPtr = std::shared_ptr<Project>;

#endif // PROJECT_H
