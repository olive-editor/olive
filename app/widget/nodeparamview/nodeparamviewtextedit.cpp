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

#include "nodeparamviewtextedit.h"

#include <QHBoxLayout>

#include "dialog/text/text.h"
#include "dialog/codeeditor/codeeditordialog.h"
#include "dialog/codeeditor/externaleditorproxy.h"
#include "dialog/codeeditor/messagehighlighter.h"
#include "ui/icons/icons.h"
#include "config/config.h"


namespace olive {

NodeParamViewTextEdit::NodeParamViewTextEdit(QWidget *parent) :
  QWidget(parent),
  code_editor_flag_(false),
  code_issues_flag_(false)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  line_edit_ = new QPlainTextEdit();
  line_edit_->setUndoRedoEnabled(true);
  connect(line_edit_, &QPlainTextEdit::textChanged, this, &NodeParamViewTextEdit::InnerWidgetTextChanged);
  layout->addWidget(line_edit_);

  edit_btn_ = new QPushButton();
  edit_btn_->setIcon(icon::ToolEdit);
  edit_btn_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
  layout->addWidget(edit_btn_);
  connect(edit_btn_, &QPushButton::clicked, this, &NodeParamViewTextEdit::ShowTextDialog);

  edit_in_viewer_btn_ = new QPushButton(tr("Edit In Viewer"));
  edit_in_viewer_btn_->setIcon(icon::Pencil);
  layout->addWidget(edit_in_viewer_btn_);
  connect(edit_in_viewer_btn_, &QPushButton::clicked, this, &NodeParamViewTextEdit::RequestEditInViewer);

  SetEditInViewerOnlyMode(false);

  ext_editor_proxy_ = new ExternalEditorProxy( this);
  connect( ext_editor_proxy_, & ExternalEditorProxy::textChanged,
           this, & NodeParamViewTextEdit::OnTextChangedExternally);
}

void NodeParamViewTextEdit::SetEditInViewerOnlyMode(bool on)
{
  line_edit_->setVisible(!on);
  edit_btn_->setVisible(!on);
  edit_in_viewer_btn_->setVisible(on);
}


void NodeParamViewTextEdit::ShowTextDialog()
{
  QString text;

  if (code_editor_flag_) {

    launchCodeEditor(text);
  }
  else {
    TextDialog d(this->text(), this);

    if (code_issues_flag_) {
      d.setSyntaxHighlight( new MessageSyntaxHighlighter());
    }

    if (d.exec() == QDialog::Accepted) {
      text= d.text();
    }
  }

  if (text != QString()) {
    setText(text);
    emit textEdited( text);
  }
}

void olive::NodeParamViewTextEdit::launchCodeEditor(QString & text)
{
  if (Config::Current()["EditorUseInternal"].toBool()) {

    // internal editor
    CodeEditorDialog d(this->text(), this);

    if (d.exec() == QDialog::Accepted) {
      text = d.text();
    }
  }
  else {

    // external editor
    ext_editor_proxy_->launch( this->text());
  }
}

void NodeParamViewTextEdit::InnerWidgetTextChanged()
{
  emit textEdited(this->text());
}

void NodeParamViewTextEdit::OnTextChangedExternally(const QString &new_text)
{
  line_edit_->setPlainText( new_text);
  emit textEdited( new_text);
}

void NodeParamViewTextEdit::setCodeEditorFlag()
{
  code_editor_flag_ = true;
  setProperty("is_exapandable", QVariant::fromValue<bool>(true));

  // if the text box is a shader code editor, make it read only so that
  // the shader code is not re-parsed on every key pressed by the user.
  // Please use the Text Dialog to edit code.
  line_edit_->setReadOnly( true);
}

void NodeParamViewTextEdit::setCodeIssuesFlag()
{
  code_issues_flag_ = true;
  line_edit_->setReadOnly( true);
  setProperty("is_exapandable", QVariant::fromValue<bool>(true));

  new MessageSyntaxHighlighter( line_edit_->document());
}

}
