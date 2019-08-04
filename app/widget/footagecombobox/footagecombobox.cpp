#include "footagecombobox.h"

#include <QAction>
#include <QDebug>
#include <QMenu>

FootageComboBox::FootageComboBox(QWidget *parent) :
  QComboBox(parent),
  root_(nullptr),
  footage_(nullptr)
{

}

void FootageComboBox::showPopup()
{
  if (root_ == nullptr || root_->child_count() == 0) {
    return;
  }

  QMenu menu;

  menu.setMinimumWidth(width());

  TraverseFolder(root_, &menu);

  QAction* selected = menu.exec(parentWidget()->mapToGlobal(pos()));

  if (selected != nullptr) {
    // Use combobox functions to show the footage name
    clear();

    addItem(selected->text());

    footage_ = reinterpret_cast<Footage*>(selected->data().value<quintptr>());

    emit FootageChanged(footage_);
  }
}

void FootageComboBox::SetRoot(const Folder *p)
{
  root_ = p;

  clear();
}

Footage *FootageComboBox::SelectedFootage()
{
  return footage_;
}

void FootageComboBox::SetFootage(Footage *f)
{
  // Remove existing single item used to show the footage name
  clear();

  footage_ = f;

  if (footage_ != nullptr) {
    // Use combobox functions to show the footage name
    addItem(footage_->name());
  }
}

void FootageComboBox::TraverseFolder(const Folder *f, QMenu *m)
{
  for (int i=0;i<f->child_count();i++) {
    Item* child = f->child(i);

    if (child->CanHaveChildren()) {

      TraverseFolder(static_cast<Folder*>(child), m->addMenu(child->name()));

    } else if (child->type() == Item::kFootage) {

      QAction* footage_action = m->addAction(child->name());
      footage_action->setData(reinterpret_cast<quintptr>(child));

    }
  }
}
