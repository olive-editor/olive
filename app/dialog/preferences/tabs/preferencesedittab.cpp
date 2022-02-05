/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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


#include "common/filefunctions.h"

namespace olive {

PreferencesEditTab::PreferencesEditTab()
{
  QVBoxLayout* outer_layout = new QVBoxLayout(this);

  QGroupBox* selection_group = new QGroupBox(tr("Select editor"));
  outer_layout->addWidget(selection_group);

  QVBoxLayout* editor_slect_layout = new QVBoxLayout(selection_group);
  use_internal_editor_ = new QRadioButton(tr("Use internal code editor"));
  use_external_editor_ = new QRadioButton(tr("Use an external code editor"));

  editor_slect_layout->addWidget(use_internal_editor_);
  editor_slect_layout->addWidget(use_external_editor_);

  internal_editor_box_ = new QGroupBox(tr("Internal editor"));
  external_editor_box_ = new QGroupBox(tr("External editor"));
  outer_layout->addWidget(internal_editor_box_);
  outer_layout->addWidget(external_editor_box_);

  connect( use_internal_editor_, & QRadioButton::clicked,
           internal_editor_box_, & QGroupBox::setEnabled);
  connect( use_internal_editor_, & QRadioButton::clicked,
           external_editor_box_, & QGroupBox::setDisabled);
  connect( use_external_editor_, & QRadioButton::clicked,
           external_editor_box_, & QGroupBox::setEnabled);
  connect( use_external_editor_, & QRadioButton::clicked,
           internal_editor_box_, & QGroupBox::setDisabled);

  //internal editor
  QGridLayout * internal_editor_layout = new QGridLayout(internal_editor_box_);
  internal_editor_layout->addWidget( new QLabel(tr("Font size")), 1, 1);
  font_size_ = new QSpinBox();
  internal_editor_layout->addWidget( font_size_, 1, 2);
  internal_editor_layout->addWidget( new QLabel(tr("Indent size")), 2, 1);
  indent_size_ = new QSpinBox();
  internal_editor_layout->addWidget( indent_size_, 2, 2);
  internal_editor_layout->addItem( new QSpacerItem(1,1, QSizePolicy::Expanding), 1, 3);
  internal_editor_layout->addItem( new QSpacerItem(1,1, QSizePolicy::Expanding), 2, 3);
  outer_layout->addWidget(internal_editor_box_);

  //external editor
  QVBoxLayout * external_editor_layout = new QVBoxLayout(external_editor_box_);
  external_editor_layout->addWidget(
        new QLabel(tr("<p>Command to launch external editor.</p>"
                      "<p>Use <i>%FILE</i> for text file path and <i>%LINE</i> for line number.</p>")));
  ext_command_ = new QLineEdit();
  external_editor_layout->addWidget( ext_command_);
  outer_layout->addWidget(external_editor_box_);

  outer_layout->addStretch();

  bool use_internal = Config::Current()["EditorUseInternal"].toBool();
  use_internal_editor_->setChecked( use_internal);
  use_external_editor_->setChecked( ! use_internal);
  internal_editor_box_->setEnabled( use_internal);
  external_editor_box_->setEnabled( ! use_internal);
  font_size_->setValue( Config::Current()["EditorInternalFontSize"].toInt());
  indent_size_->setValue( Config::Current()["EditorInternalIndentSize"].toInt());
  ext_command_->setText( Config::Current()["EditorExternalCommand"].toString());
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
  Config::Current()["EditorInternalFontSize"] = QVariant::fromValue(font_size_->value());
  Config::Current()["EditorInternalIndentSize"] = QVariant::fromValue(indent_size_->value());
}

}
