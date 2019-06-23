#include "project.h"

Project::Project()
{
  name_ = tr("(untitled)");
}

Folder *Project::root()
{
  return &root_;
}

const QString &Project::name()
{
  return name_;
}

void Project::set_name(const QString &s)
{
  name_ = s;
}
