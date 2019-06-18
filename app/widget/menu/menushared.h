#ifndef MENU_H
#define MENU_H

#include "widget/menu/menu.h"

class MenuShared : public QObject {
public:
  MenuShared();

  void Initialize();
  void Retranslate();

  void AddItemsForNewMenu(Menu* m);
  void AddItemsForEditMenu(Menu* m);
  void AddItemsForInOutMenu(Menu* m);
  void AddItemsForClipEditMenu(Menu* m);

private:
  // "New" menu shared items
  QAction* new_project_item_;
  QAction* new_sequence_item_;
  QAction* new_folder_item_;

  // "Edit" menu shared items
  QAction* edit_cut_item_;
  QAction* edit_copy_item_;
  QAction* edit_paste_item_;
  QAction* edit_paste_insert_item_;
  QAction* edit_duplicate_item_;
  QAction* edit_delete_item_;
  QAction* edit_ripple_delete_item_;
  QAction* edit_split_item_;

  // "In/Out" menu shared items
  QAction* inout_set_in_item_;
  QAction* inout_set_out_item_;
  QAction* inout_reset_in_item_;
  QAction* inout_reset_out_item_;
  QAction* inout_clear_inout_item_;

  // "Clip Edit" menu shared items
  QAction* clip_add_default_transition_item_;
  QAction* clip_link_unlink_item_;
  QAction* clip_enable_disable_item_;
  QAction* clip_nest_item_;
};

namespace olive {
extern MenuShared menu_shared;
}

#endif // MENU_H
