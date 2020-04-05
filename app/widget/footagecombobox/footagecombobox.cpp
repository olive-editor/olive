#include "footagecombobox.h"

#include <QAction>
#include <QDebug>

#include "ui/icons/icons.h"
#include "widget/menu/menu.h"

FootageComboBox::FootageComboBox(QWidget *parent) :
  QComboBox(parent),
  root_(nullptr),
  footage_(nullptr),
  only_show_ready_footage_(true)
{
}

void FootageComboBox::showPopup()
{
  if (root_ == nullptr || root_->child_count() == 0) {
    return;
  }

  Menu menu;

  menu.setMinimumWidth(width());

  TraverseFolder(root_, &menu);

  QAction* selected = menu.exec(parentWidget()->mapToGlobal(pos()));

  if (selected != nullptr) {
    // Use combobox functions to show the footage name
    clear();

    addItem(selected->text());

    footage_ = selected->data().value<StreamPtr>();

    emit FootageChanged(footage_);
  }
}

void FootageComboBox::SetRoot(const Folder *p)
{
  root_ = p;

  clear();
}

void FootageComboBox::SetOnlyShowReadyFootage(bool e)
{
  only_show_ready_footage_ = e;
}

StreamPtr FootageComboBox::SelectedFootage()
{
  return footage_;
}

void FootageComboBox::SetFootage(StreamPtr f)
{
  // Remove existing single item used to show the footage name
  clear();

  footage_ = f;

  if (footage_ != nullptr) {
    // Use combobox functions to show the footage name
    addItem(footage_->footage()->name());
  }
}

void FootageComboBox::TraverseFolder(const Folder *f, QMenu *m)
{
  for (int i=0;i<f->child_count();i++) {
    Item* child = f->child(i);

    if (child->CanHaveChildren()) {

      TraverseFolder(static_cast<Folder*>(child), m->addMenu(child->name()));

    } else if (child->type() == Item::kFootage) {

      Footage* footage = static_cast<Footage*>(child);

      if (!only_show_ready_footage_ || footage->status() == Footage::kReady) {
        QMenu* stream_menu = m->addMenu(footage->name());

        foreach (StreamPtr stream, footage->streams()) {
          QAction* stream_action = stream_menu->addAction(stream->description());
          stream_action->setData(QVariant::fromValue(stream));
          stream_action->setIcon(Stream::IconFromType(stream->type()));
        }
      }
    }
  }
}
