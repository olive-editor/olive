/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "configdialogbase.h"

#include <QDialogButtonBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QAbstractButton>

#include "core.h"

namespace olive {

ConfigDialogBase::ConfigDialogBase(QWidget* parent) :
  QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QSplitter* splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  list_widget_ = new QListWidget();

  preference_pane_stack_ = new QStackedWidget(this);

  splitter->addWidget(list_widget_);
  splitter->addWidget(preference_pane_stack_);

  QDialogButtonBox* button_box = new QDialogButtonBox(this);
  button_box->setOrientation(Qt::Horizontal);
  button_box->setStandardButtons(QDialogButtonBox::Apply | QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

  layout->addWidget(button_box);

  connect(button_box, &QDialogButtonBox::accepted, this, [this]() {apply(); QDialog::accept();});;
  connect(button_box, &QDialogButtonBox::rejected, this, &ConfigDialogBase::reject);
  connect(button_box, &QDialogButtonBox::clicked, this, [this, button_box](QAbstractButton* btn) {if (button_box->buttonRole(btn) == QDialogButtonBox::ApplyRole) apply();});

  connect(list_widget_,
          &QListWidget::currentRowChanged,
          preference_pane_stack_,
          &QStackedWidget::setCurrentIndex);
}

void ConfigDialogBase::apply()
{
  foreach (ConfigDialogBaseTab* tab, tabs_) {
    if (!tab->Validate()) {
      return;
    }
  }

  MultiUndoCommand* command = new MultiUndoCommand();

  foreach (ConfigDialogBaseTab* tab, tabs_) {
    tab->Accept(command);
  }

  Core::instance()->undo_stack()->pushIfHasChildren(command);

  AcceptEvent();

}

void ConfigDialogBase::AddTab(ConfigDialogBaseTab *tab, const QString &title)
{
  list_widget_->addItem(title);
  preference_pane_stack_->addWidget(tab);

  tabs_.append(tab);
}

}
