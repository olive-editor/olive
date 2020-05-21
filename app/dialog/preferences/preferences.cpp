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

#include "preferences.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QApplication>

#include "config/config.h"
#include "tabs/preferencesgeneraltab.h"
#include "tabs/preferencesbehaviortab.h"
#include "tabs/preferencesappearancetab.h"
#include "tabs/preferencesqualitytab.h"
#include "tabs/preferencesdisktab.h"
#include "tabs/preferencesaudiotab.h"
#include "tabs/preferenceskeyboardtab.h"

OLIVE_NAMESPACE_ENTER

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
  AddTab(new PreferencesQualityTab(), tr("Quality"));
  AddTab(new PreferencesDiskTab(), tr("Disk"));
  AddTab(new PreferencesAudioTab(), tr("Audio"));
  AddTab(new PreferencesKeyboardTab(main_menu_bar), tr("Keyboard"));

  // Set default selected tab
  list_widget_->setCurrentRow(0);

  splitter->addWidget(list_widget_);
  splitter->addWidget(preference_pane_stack_);

  QDialogButtonBox* button_box = new QDialogButtonBox(this);
  button_box->setOrientation(Qt::Horizontal);
  button_box->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok | QDialogButtonBox::RestoreDefaults);
  button_box->button(QDialogButtonBox::RestoreDefaults)
      ->setToolTip(tr("Reset tab to default settings\nCtrl click resets all preferences"));

  layout->addWidget(button_box);

  connect(button_box, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);

  connect(button_box->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
          &PreferencesDialog::RestoreDefaultSettings);

  connect(list_widget_,
          &QListWidget::currentRowChanged,
          preference_pane_stack_,
          &QStackedWidget::setCurrentIndex);

  connect(this, &PreferencesDialog::ResetGeneral,
          static_cast<olive::PreferencesGeneralTab*>(preference_pane_stack_->widget(0)),
          &PreferencesGeneralTab::ResetDefaults);

  connect(this, &PreferencesDialog::ResetAppearance,
          static_cast<olive::PreferencesAppearanceTab*>(preference_pane_stack_->widget(1)),
          &PreferencesAppearanceTab::ResetDefaults);

  connect(this, &PreferencesDialog::ResetBehavior,
          static_cast<olive::PreferencesBehaviorTab*>(preference_pane_stack_->widget(2)),
          &PreferencesBehaviorTab::ResetDefaults);

  connect(this, &PreferencesDialog::ResetQuality,
          static_cast<olive::PreferencesQualityTab*>(preference_pane_stack_->widget(3)),
          &PreferencesQualityTab::ResetDefaults);

  connect(this, &PreferencesDialog::ResetDisk,
          static_cast<olive::PreferencesDiskTab*>(preference_pane_stack_->widget(4)),
          &PreferencesDiskTab::ResetDefaults);

  connect(this, &PreferencesDialog::ResetAudio,
          static_cast<olive::PreferencesAudioTab*>(preference_pane_stack_->widget(5)),
          &PreferencesAudioTab::ResetDefaults);

  connect(this, &PreferencesDialog::ResetKeyboard,
          static_cast<olive::PreferencesKeyboardTab*>(preference_pane_stack_->widget(6)),
          &PreferencesKeyboardTab::ResetDefaults);
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

void PreferencesDialog::RestoreDefaultSettings() 
{ 
  bool reset_all_tabs = false;

  if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier)) {
    if(QMessageBox::question(this, tr("Confirm Reset All Settings"),
                                                 tr("Are you sure you wish to reset all settings?"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      reset_all_tabs = true;
    } else {
      return;
    }
  }

  QString list_item = list_widget_->currentItem()->text();
  if (list_item == tr("General") || reset_all_tabs) {
    emit ResetGeneral(reset_all_tabs);
  }
  if (list_item == tr("Appearance") || reset_all_tabs) {
    emit ResetAppearance(reset_all_tabs);
  }
  if (list_item == tr("Behavior") || reset_all_tabs) {
    emit ResetBehavior(reset_all_tabs);
  }
  if (list_item == tr("Quality") || reset_all_tabs) {
    emit ResetQuality(reset_all_tabs);
  }
  if (list_item == tr("Disk") || reset_all_tabs) {
    emit ResetDisk(reset_all_tabs);
  }
  if (list_item == tr("Audio") || reset_all_tabs) {
    emit ResetAudio(reset_all_tabs);
  }
  if (list_item == tr("Keyboard") || reset_all_tabs) {
    emit ResetKeyboard(reset_all_tabs);
  }
}

void PreferencesDialog::AddTab(PreferencesTab *tab, const QString &title)
{
  list_widget_->addItem(title);
  preference_pane_stack_->addWidget(tab);

  tabs_.append(tab);
}

OLIVE_NAMESPACE_EXIT
