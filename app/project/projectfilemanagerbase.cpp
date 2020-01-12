#include "projectfilemanagerbase.h"

ProjectFileManagerBase::ProjectFileManagerBase() :
  cancelled_(false)
{

}

void ProjectFileManagerBase::Cancel()
{
  cancelled_ = true;
}
