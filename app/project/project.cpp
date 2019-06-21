#include "project.h"

Project::Project()
{
}

Folder *Project::root()
{
  return &root_;
}
