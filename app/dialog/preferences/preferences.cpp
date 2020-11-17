/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "preferences.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>

#include "config/config.h"
#include "tabs/preferencesgeneraltab.h"
#include "tabs/preferencesbehaviortab.h"
#include "tabs/preferencesappearancetab.h"
#include "tabs/preferencesdisktab.h"
#include "tabs/preferencesaudiotab.h"
#include "tabs/preferenceskeyboardtab.h"

namespace olive {

PreferencesDialog::PreferencesDialog(QWidget *parent, QMenuBar* main_menu_bar) :
  QDialog(parent)
{
  setWindowTitle(tr("Preferences"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  QSplitter* splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  layout->addWidget(splitter);

  list_widget_ = new QListWidget();

  preference_pane_stack_ = new QStackedWidget(this);

  AddTab(new PreferencesGeneralTab(), tr("General"));
  AddTab(new PreferencesAppearanceTab(), tr("Appearance"));
  AddTab(new PreferencesBehaviorTab(), tr("Behavior"));
  AddTab(new PreferencesDiskTab(), tr("Disk"));
  AddTab(new PreferencesAudioTab(), tr("Audio"));
  AddTab(new PreferencesKeyboardTab(main_menu_bar), tr("Keyboard"));

  splitter->addWidget(list_widget_);
  splitter->addWidget(preference_pane_stack_);

  QDialogButtonBox* button_box = new QDialogButtonBox(this);
  button_box->setOrientation(Qt::Horizontal);
  button_box->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

  layout->addWidget(button_box);

  connect(button_box, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);

  connect(list_widget_,
          &QListWidget::currentRowChanged,
          preference_pane_stack_,
          &QStackedWidget::setCurrentIndex);
}

void PreferencesDialog::accept()
{
  foreach (PreferencesTab* tab, tabs_) {
    if (!tab->Validate()) {
      return;
    }
  }

  foreach (PreferencesTab* tab, tabs_) {
    tab->Accept();
  }

  QDialog::accept();
}

void PreferencesDialog::AddTab(PreferencesTab *tab, const QString &title)
{
  list_widget_->addItem(title);
  preference_pane_stack_->addWidget(tab);

  tabs_.append(tab);
}

}
