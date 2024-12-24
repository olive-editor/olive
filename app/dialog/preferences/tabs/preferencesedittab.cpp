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

#include "preferencesedittab.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>


namespace olive {

PreferencesEditTab::PreferencesEditTab()
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGroupBox* selection_group = new QGroupBox(tr("Select editor"));
  outer_layout->addWidget(selection_group);
  QGroupBox * internal_editor_box = new QGroupBox(tr("Internal editor"));
  outer_layout->addWidget(internal_editor_box);
  QGroupBox * external_editor_box = new QGroupBox(tr("External editor"));
  outer_layout->addWidget(external_editor_box);

  QVBoxLayout* editor_slect_layout = new QVBoxLayout(selection_group);
  use_internal_editor_ = new QRadioButton(tr("Use internal code editor"));
  use_external_editor_ = new QRadioButton(tr("Use an external code editor"));

  editor_slect_layout->addWidget(use_internal_editor_);
  editor_slect_layout->addWidget(use_external_editor_);

  connect( use_internal_editor_, & QRadioButton::clicked,
           internal_editor_box, & QGroupBox::setEnabled);
  connect( use_internal_editor_, & QRadioButton::clicked,
           external_editor_box, & QGroupBox::setDisabled);
  connect( use_external_editor_, & QRadioButton::clicked,
           external_editor_box, & QGroupBox::setEnabled);
  connect( use_external_editor_, & QRadioButton::clicked,
           internal_editor_box, & QGroupBox::setDisabled);

  //internal editor
  QGridLayout * internal_editor_layout = new QGridLayout(internal_editor_box);
  internal_editor_layout->addWidget( new QLabel(tr("Font size")), 0, 0);
  font_size_ = new QSpinBox();
  internal_editor_layout->addWidget( font_size_, 0, 1);
  internal_editor_layout->addWidget( new QLabel(tr("Indent size")), 1, 0);
  indent_size_ = new QSpinBox();
  internal_editor_layout->addWidget( indent_size_, 1, 1);
  internal_editor_layout->addWidget( new QLabel(tr("Window size (WxH)")), 2, 0);
  window_width_ = new QSpinBox();
  window_heigth_ = new QSpinBox();
  internal_editor_layout->addWidget( window_width_, 2, 1);
  internal_editor_layout->addWidget( window_heigth_, 2, 2);
  internal_editor_layout->addItem( new QSpacerItem(1,1, QSizePolicy::Expanding), 0, 3);
  internal_editor_layout->addItem( new QSpacerItem(1,1, QSizePolicy::Expanding), 1, 3);
  internal_editor_layout->addItem( new QSpacerItem(1,1, QSizePolicy::Expanding), 2, 3);
  outer_layout->addWidget(internal_editor_box);

  window_heigth_->setMinimum(10);
  window_heigth_->setMaximum(4096);
  window_width_->setMinimum(10);
  window_width_->setMaximum(4096);

  //external editor
  QVBoxLayout * external_editor_layout = new QVBoxLayout(external_editor_box);
  external_editor_layout->addWidget(
        new QLabel(tr("Command or full file path of external editor.\n"
                      "Use double quotes if executable path has spaces")) );
  QHBoxLayout * editor_cmd_layout = new QHBoxLayout();
  external_editor_layout->addLayout( editor_cmd_layout);
  ext_command_ = new QLineEdit();
  ext_select_button = new QPushButton("...", this);
  editor_cmd_layout->addWidget( ext_command_);
  editor_cmd_layout->addWidget( ext_select_button);

  connect( ext_select_button, & QPushButton::clicked, this, & PreferencesEditTab::onCommandSelectDialogRequest);

  ext_params_ = new QLineEdit();
  external_editor_layout->addWidget(
        new QLabel(tr("Parameters of external editor.\n"
                      "Use %FILE and %LINE for file path and line number")) );
  external_editor_layout->addWidget( ext_params_);
  outer_layout->addWidget(external_editor_box);

  external_editor_layout->addWidget(
      new QLabel(tr("Folder where temporary shaders are stored")) );
  QHBoxLayout * temp_folder_layout = new QHBoxLayout();
  external_editor_layout->addLayout( temp_folder_layout);
  temp_folder_ = new QLineEdit();
  temp_folder_button_ = new QPushButton("...", this);
  temp_folder_layout->addWidget( temp_folder_);
  temp_folder_layout->addWidget( temp_folder_button_);

  connect( temp_folder_button_, & QPushButton::clicked, this, & PreferencesEditTab::onTempFolderDialogRequest);

  outer_layout->addStretch();

  bool use_internal = Config::Current()["EditorUseInternal"].toBool();
  use_internal_editor_->setChecked( use_internal);
  use_external_editor_->setChecked( ! use_internal);
  internal_editor_box->setEnabled( use_internal);
  external_editor_box->setEnabled( ! use_internal);
  font_size_->setValue( Config::Current()["EditorInternalFontSize"].toInt());
  indent_size_->setValue( Config::Current()["EditorInternalIndentSize"].toInt());
  ext_command_->setText( Config::Current()["EditorExternalCommand"].toString());
  ext_params_->setText( Config::Current()["EditorExternalParams"].toString());
  temp_folder_->setText( Config::Current()["EditorExternalTempFolder"].toString());
  window_heigth_->setValue( Config::Current()["EditorInternalWindowHeight"].toInt());
  window_width_->setValue( Config::Current()["EditorInternalWindowWidth"].toInt());
}

bool PreferencesEditTab::Validate()
{
  return true;
}

void PreferencesEditTab::Accept(MultiUndoCommand *command)
{
  Q_UNUSED(command)

  Config::Current()["EditorUseInternal"] = QVariant::fromValue( use_internal_editor_->isChecked());
  Config::Current()["EditorExternalCommand"] = QVariant::fromValue(ext_command_->text());
  Config::Current()["EditorExternalParams"] = QVariant::fromValue(ext_params_->text());
  Config::Current()["EditorExternalTempFolder"] = QVariant::fromValue(temp_folder_->text());
  Config::Current()["EditorInternalFontSize"] = QVariant::fromValue(font_size_->value());
  Config::Current()["EditorInternalIndentSize"] = QVariant::fromValue(indent_size_->value());
  Config::Current()["EditorInternalWindowHeight"] = QVariant::fromValue(window_heigth_->value());
  Config::Current()["EditorInternalWindowWidth"] = QVariant::fromValue(window_width_->value());
}

void PreferencesEditTab::onCommandSelectDialogRequest()
{
  QString path = QFileDialog::getOpenFileName( this, tr("External editor"),
                                               QStandardPaths::displayName( QStandardPaths::ApplicationsLocation));

  if (path != QString()) {
    ext_command_->setText( path);
  }
}

void PreferencesEditTab::onTempFolderDialogRequest()
{
  QString path = QFileDialog::getExistingDirectory( this, tr("Temporary shaders folder"),
                                                    Config::Current()["EditorExternalTempFolder"].toString());

  if (path != QString()) {
    temp_folder_->setText( path);
  }
}

}  // olive
