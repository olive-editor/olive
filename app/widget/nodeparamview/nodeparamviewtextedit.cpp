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

#include "dialog/codeeditor/messagehighlighter.h"
#include "ui/icons/icons.h"
#include "nodeparamviewshader.h"


namespace olive {

NodeParamViewTextEdit::NodeParamViewTextEdit(QWidget *parent) :
  QWidget(parent),
  code_editor_flag_(false),
  code_issues_flag_(false),
  text_dlg_(nullptr)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  line_edit_ = new QPlainTextEdit();
  line_edit_->setUndoRedoEnabled(true);
  connect(line_edit_, &QPlainTextEdit::textChanged, this, &NodeParamViewTextEdit::InnerWidgetTextChanged);
  layout->addWidget(line_edit_);

  shader_edit_ = new NodeParamViewShader( *line_edit_, this);
  layout->addWidget( shader_edit_->widget());
  connect( shader_edit_, & NodeParamViewShader::OnTextChangedExternally,
          this, & NodeParamViewTextEdit::OnTextChangedExternally);

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
}

NodeParamViewTextEdit::~NodeParamViewTextEdit()
{
  // This prevents a crash if text changes externally while
  // the dialog is open. For example this happens when code issues dialog
  // is open and shader code changes externally.
  if (text_dlg_ != nullptr) {
    text_dlg_->reject();
  }
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

    shader_edit_->launchCodeEditor(text);
  } else {
    text_dlg_ = new TextDialog(this->text(), nullptr);

    if (code_issues_flag_) {
      text_dlg_->setSyntaxHighlight( new MessageSyntaxHighlighter());
    }

    if (text_dlg_->exec() == QDialog::Accepted) {
      text= text_dlg_->text();
    }

    delete text_dlg_;
    text_dlg_ = nullptr;
  }

  if (text != QString()) {
    setText(text);
    emit textEdited( text);
  }
}


void NodeParamViewTextEdit::InnerWidgetTextChanged()
{
  emit textEdited(this->text());
}

void NodeParamViewTextEdit::OnTextChangedExternally(const QString & content)
{
  line_edit_->setPlainText( content);
  emit textEdited( content);
}

void NodeParamViewTextEdit::setShaderCodeEditorFlag( const Node * owner, bool is_vertex)
{
  code_editor_flag_ = true;

  // no need to draw the stopwatch to keyframe this input
  setProperty("is_exapandable", QVariant::fromValue<bool>(true));

  // as this is a shader code editor, it is edited by a
  // separate dialog or external editor
  line_edit_->setReadOnly( true);
  line_edit_->setVisible(false);
  shader_edit_->attachOwnerNode( owner, is_vertex);
}

void NodeParamViewTextEdit::setShaderIssuesFlag()
{
  code_issues_flag_ = true;
  line_edit_->setReadOnly( true);

  // no need to draw the stopwatch to keyframe this input
  setProperty("is_exapandable", QVariant::fromValue<bool>(true));

  new MessageSyntaxHighlighter( line_edit_->document());
}

}
