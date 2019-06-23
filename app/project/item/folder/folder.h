#ifndef FOLDER_H
#define FOLDER_H

#include "project/item/item.h"

class Folder : public Item
{
public:
  Folder();

  virtual Type type() const override;

private:

};

#endif // FOLDER_H
