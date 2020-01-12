#include "projectsavemanager.h"

ProjectSaveManager::ProjectSaveManager(Project *project) :
  project_(project)
{

}

void ProjectSaveManager::Start()
{
  int prog = 0;

  do {
    prog += 10;

    emit ProgressChanged(prog);

    Sleep(200);
  } while (prog < 100);

  emit Finished();
}
