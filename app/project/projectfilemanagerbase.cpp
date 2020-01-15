#include "projectfilemanagerbase.h"

ProjectFileManagerBase::ProjectFileManagerBase() :
  cancelled_(false)
{

}

void ProjectFileManagerBase::Start()
{
  Action();

  emit Finished();
}

void ProjectFileManagerBase::Cancel()
{
  cancelled_ = true;
}

const QAtomicInt &ProjectFileManagerBase::IsCancelled() const
{
  return cancelled_;
}
