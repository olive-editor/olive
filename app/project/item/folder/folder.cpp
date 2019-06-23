#include "folder.h"

Folder::Folder()
{

}

Item::Type Folder::type() const
{
  return kFolder;
}
