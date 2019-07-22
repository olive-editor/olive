#include "footagecombobox.h"

#include <QAction>
#include <QMenu>

FootageComboBox::FootageComboBox(QWidget *parent) :
  QComboBox(parent),
  root_(nullptr)
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
    clear();

    addItem(selected->text());

    emit FootageChanged(reinterpret_cast<Footage*>(selected->data().value<quintptr>()));
  }
}

void FootageComboBox::SetRoot(const Folder *p)
{
  root_ = p;

  clear();
}

void FootageComboBox::SetFootage(Footage *f)
{
  clear();

  addItem(f->name());
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
