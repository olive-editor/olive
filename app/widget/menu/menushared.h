#ifndef MENU_H
#define MENU_H

#include "widget/menu/menu.h"

class MenuShared : public QObject {
public:
  MenuShared();

  void Initialize();

  void AddItemsForNewMenu(Menu* m);

private:
  void Retranslate();

  QAction* new_project_item_;
  QAction* new_sequence_item_;
  QAction* new_folder_item_;
};

namespace olive {
extern MenuShared menu_shared;
}

#endif // MENU_H
