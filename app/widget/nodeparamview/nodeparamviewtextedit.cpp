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

#include "nodeparamviewtextedit.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "dialog/text/text.h"
#include "dialog/codeeditor/codeeditordialog.h"
#include "ui/icons/icons.h"

#include <qdebug.h>
namespace olive {

NodeParamViewTextEdit::NodeParamViewTextEdit(QWidget *parent) :
  QWidget(parent),
  code_editor_flag_(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);

  line_edit_ = new QPlainTextEdit();
  line_edit_->setUndoRedoEnabled(true);
  connect(line_edit_, &QPlainTextEdit::textChanged, this, &NodeParamViewTextEdit::InnerWidgetTextChanged);
  layout->addWidget(line_edit_);

  QPushButton* edit_btn = new QPushButton();
  edit_btn->setIcon(icon::ToolEdit);
  edit_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
  layout->addWidget(edit_btn);
  connect(edit_btn, &QPushButton::clicked, this, &NodeParamViewTextEdit::ShowTextDialog);
}

void NodeParamViewTextEdit::ShowTextDialog()
{
  QString text;

  if (code_editor_flag_) {
    CodeEditorDialog d(this->text(), this);

    if (d.exec() == QDialog::Accepted) {
      text = d.text();
    }
  }
  else {
    TextDialog d(this->text(), this);

    if (d.exec() == QDialog::Accepted) {
      text= d.text();
    }
  }

  if (text != QString()) {
    line_edit_->setPlainText( text);
    emit textEdited( text);
  }
}

void NodeParamViewTextEdit::InnerWidgetTextChanged()
{
  emit textEdited(this->text());
}

void NodeParamViewTextEdit::setCodeEditoFlag()
{
  code_editor_flag_ = true;

  // if the text box is a shader code editor, make it read only so that
  // the shader code is not re-parsed on every key pressed by the user.
  // Please use the Text Dialog to edit code.
  line_edit_->setEnabled( false);
}

}
