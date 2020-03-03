#ifndef FOOTAGEMANAGEMENTPANEL_H
#define FOOTAGEMANAGEMENTPANEL_H

#include <QList>

#include "project/item/footage/footage.h"

class FootageManagementPanel {
public:
  virtual QList<Footage*> GetSelectedFootage() = 0;
};

#endif // FOOTAGEMANAGEMENTPANEL_H
