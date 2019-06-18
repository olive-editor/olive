#include "mainmenu.h"

#include <QEvent>

#include "widget/menu/menushared.h"

MainMenu::MainMenu(QWidget *parent) :
  QMenuBar(parent)
{
  //
  // FILE MENU
  //
  file_menu_ = new Menu(this);
  file_new_menu_ = new Menu(file_menu_);
  olive::menu_shared.AddItemsForNewMenu(file_new_menu_);
  file_open_item_ = file_menu_->AddItem("openproj", nullptr, nullptr, "Ctrl+O");
  file_open_recent_menu_ = new Menu(file_menu_);
  file_open_recent_clear_item_ = file_open_recent_menu_->AddItem("clearopenrecent", nullptr, nullptr);
  file_save_item_ = file_menu_->AddItem("saveproj", nullptr, nullptr, "Ctrl+S");
  file_save_as_item_ = file_menu_->AddItem("saveprojas", nullptr, nullptr, "Ctrl+Shift+S");
  file_menu_->addSeparator();
  file_import_item_ = file_menu_->AddItem("import", nullptr, nullptr, "Ctrl+I");
  file_menu_->addSeparator();
  file_export_item_ = file_menu_->AddItem("export", nullptr, nullptr, "Ctrl+M");
  file_menu_->addSeparator();
  file_exit_item_ = file_menu_->AddItem("exit", nullptr, nullptr);

  //
  // EDIT MENU
  //
  edit_menu_ = new Menu(this);
  edit_undo_item_ = edit_menu_->AddItem("undo", nullptr, nullptr, "Ctrl+Z");
  edit_redo_item_ = edit_menu_->AddItem("redo", nullptr, nullptr, "Ctrl+Shift+Z");

  //
  // VIEW MENU
  //
  view_menu_ = new Menu(this);

  //
  // PLAYBACK MENU
  //
  playback_menu_ = new Menu(this);

  //
  // WINDOW MENU
  //
  window_menu_ = new Menu(this);

  //
  // HELP MENU
  //
  tools_menu_ = new Menu(this);

  //
  // FILE MENU
  //
  help_menu_ = new Menu(this);

  Retranslate();
}

void MainMenu::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QMenuBar::changeEvent(e);
}

void MainMenu::Retranslate()
{
  file_menu_->setTitle(tr("&File"));
  file_new_menu_->setTitle(tr("&New"));
  file_open_item_->setText(tr("&Open Project"));
  file_open_recent_menu_->setTitle(tr("Open Recent"));
  file_open_recent_clear_item_->setText(tr("Clear Recent List"));
  file_save_item_->setText(tr("&Save Project"));
  file_save_as_item_->setText(tr("Save Project &As"));
  file_import_item_->setText(tr("&Import..."));
  file_export_item_->setText(tr("&Export..."));
  file_exit_item_->setText(tr("E&xit"));

  edit_menu_->setTitle(tr("&Edit"));
  edit_undo_item_->setText(tr("&Undo"));
  edit_redo_item_->setText(tr("Redo"));
  edit_menu_->addSeparator();

  view_menu_->setTitle(tr("&View"));
  playback_menu_->setTitle(tr("&Playback"));
  window_menu_->setTitle("&Window");
  tools_menu_->setTitle(tr("&Tools"));
  help_menu_->setTitle(tr("&Help"));
}
