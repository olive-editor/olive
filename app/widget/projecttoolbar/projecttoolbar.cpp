/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "projecttoolbar.h"

#include <QHBoxLayout>
#include <QEvent>
#include <QButtonGroup>

#include "ui/icons/icons.h"

ProjectToolbar::ProjectToolbar(QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  new_button_ = new QPushButton();
  connect(new_button_, SIGNAL(clicked(bool)), this, SIGNAL(NewClicked()));
  layout->addWidget(new_button_);

  open_button_ = new QPushButton();
  connect(open_button_, SIGNAL(clicked(bool)), this, SIGNAL(OpenClicked()));
  layout->addWidget(open_button_);

  save_button_ = new QPushButton();
  connect(save_button_, SIGNAL(clicked(bool)), this, SIGNAL(SaveClicked()));
  layout->addWidget(save_button_);

  undo_button_ = new QPushButton();
  connect(undo_button_, SIGNAL(clicked(bool)), this, SIGNAL(UndoClicked()));
  layout->addWidget(undo_button_);

  redo_button_ = new QPushButton();
  connect(redo_button_, SIGNAL(clicked(bool)), this, SIGNAL(RedoClicked()));
  layout->addWidget(redo_button_);

  search_field_ = new QLineEdit();
  search_field_->setClearButtonEnabled(true);
  connect(search_field_, SIGNAL(textChanged(const QString &)), this, SIGNAL(SearchChanged(const QString&)));
  layout->addWidget(search_field_);

  tree_button_ = new QPushButton();
  tree_button_->setCheckable(true);
  connect(tree_button_, SIGNAL(clicked(bool)), this, SLOT(ViewButtonClicked()));
  layout->addWidget(tree_button_);

  list_button_ = new QPushButton();
  list_button_->setCheckable(true);
  connect(list_button_, SIGNAL(clicked(bool)), this, SLOT(ViewButtonClicked()));
  layout->addWidget(list_button_);

  icon_button_ = new QPushButton();
  icon_button_->setCheckable(true);
  connect(icon_button_, SIGNAL(clicked(bool)), this, SLOT(ViewButtonClicked()));
  layout->addWidget(icon_button_);

  // Group Tree/List/Icon view buttons into a button group for easy exclusive-buttons
  QButtonGroup* view_button_group = new QButtonGroup(this);
  view_button_group->setExclusive(true);
  view_button_group->addButton(tree_button_);
  view_button_group->addButton(list_button_);
  view_button_group->addButton(icon_button_);

  Retranslate();
  UpdateIcons();
}

void ProjectToolbar::SetView(olive::ProjectViewType type)
{
  switch (type) {
  case olive::TreeView:
    tree_button_->setChecked(true);
    break;
  case olive::IconView:
    icon_button_->setChecked(true);
    break;
  case olive::ListView:
    list_button_->setChecked(true);
    break;
  }
}

void ProjectToolbar::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  } else if (e->type() == QEvent::StyleChange) {
    UpdateIcons();
  }
  QWidget::changeEvent(e);
}

void ProjectToolbar::Retranslate()
{
  new_button_->setToolTip(tr("New..."));
  open_button_->setToolTip(tr("Open Project"));
  save_button_->setToolTip(tr("Save Project"));
  undo_button_->setToolTip(tr("Undo"));
  redo_button_->setToolTip(tr("Redo"));

  search_field_->setPlaceholderText(tr("Search media, markers, etc."));

  tree_button_->setToolTip(tr("Switch to Tree View"));
  list_button_->setToolTip(tr("Switch to List View"));
  icon_button_->setToolTip(tr("Switch to Icon View"));
}

void ProjectToolbar::UpdateIcons()
{
  new_button_->setIcon(olive::icon::New);
  open_button_->setIcon(olive::icon::Open);
  save_button_->setIcon(olive::icon::Save);
  undo_button_->setIcon(olive::icon::Undo);
  redo_button_->setIcon(olive::icon::Redo);
  tree_button_->setIcon(olive::icon::TreeView);
  list_button_->setIcon(olive::icon::ListView);
  icon_button_->setIcon(olive::icon::IconView);
}

void ProjectToolbar::ViewButtonClicked()
{
  // Determine which view button triggered this slot and emit a signal accordingly
  if (sender() == tree_button_) {
    emit ViewChanged(olive::TreeView);
  } else if (sender() == icon_button_) {
    emit ViewChanged(olive::IconView);
  } else if (sender() == list_button_) {
    emit ViewChanged(olive::ListView);
  } else {
    // Assert that it was one of the above buttons
    Q_ASSERT(false);
  }
}
