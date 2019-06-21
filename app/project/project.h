#ifndef PROJECT_H
#define PROJECT_H

#include <memory>

#include "folder.h"

class Project
{
public:
  Project();

  Folder* root();

private:
  Folder root_;
};

using ProjectPtr = std::shared_ptr<Project>;

#endif // PROJECT_H
