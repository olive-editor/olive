#include "menushared.h"

MenuShared olive::menu_shared;

MenuShared::MenuShared()
{
}

void MenuShared::Initialize()
{
  new_project_item_ = Menu::CreateItem(this, "newproj", nullptr, nullptr, "Ctrl+N");
  new_sequence_item_ = Menu::CreateItem(this, "newseq", nullptr, nullptr, "Ctrl+Shift+N");
  new_folder_item_ = Menu::CreateItem(this, "newfolder", nullptr, nullptr);

  Retranslate();
}

void MenuShared::AddItemsForNewMenu(Menu *m)
{
  m->addAction(new_project_item_);
  m->addSeparator();
  m->addAction(new_sequence_item_);
  m->addAction(new_folder_item_);
}

void MenuShared::Retranslate()
{
  new_project_item_->setText(tr("&Project"));
  new_sequence_item_->setText(tr("&Sequence"));
  new_folder_item_->setText(tr("&Folder"));
}
