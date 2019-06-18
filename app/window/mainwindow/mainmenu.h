#ifndef MAINMENU_H
#define MAINMENU_H

#include <QMenuBar>

#include "widget/menu/menu.h"

class MainMenu : public QMenuBar
{
public:
  MainMenu(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e);

private:
  void Retranslate();

  Menu* file_menu_;
  Menu* file_new_menu_;
  QAction* file_open_item_;
  Menu* file_open_recent_menu_;
  QAction* file_open_recent_clear_item_;
  QAction* file_save_item_;
  QAction* file_save_as_item_;
  QAction* file_import_item_;
  QAction* file_export_item_;
  QAction* file_exit_item_;

  Menu* edit_menu_;
  QAction* edit_undo_item_;
  QAction* edit_redo_item_;

  Menu* view_menu_;
  Menu* playback_menu_;
  Menu* window_menu_;
  Menu* tools_menu_;
  Menu* help_menu_;
};

#endif // MAINMENU_H
