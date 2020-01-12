#include "projectsavemanager.h"

ProjectSaveManager::ProjectSaveManager(Project *project) :
  project_(project),
  cancelled_(false)
{

}

void ProjectSaveManager::Start()
{
  int prog = 0;

  do {
    if (cancelled_) {
      break;
    }

    prog += 10;

    emit ProgressChanged(prog);

    Sleep(200);
  } while (prog < 100);

  emit Finished();
}

void ProjectSaveManager::Cancel()
{
  cancelled_ = true;
}
