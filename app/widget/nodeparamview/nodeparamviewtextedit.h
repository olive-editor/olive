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

#ifndef NODEPARAMVIEWTEXTEDIT_H
#define NODEPARAMVIEWTEXTEDIT_H

#include <QPlainTextEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QWidget>

#include "common/define.h"


namespace olive {

class Node;
class NodeParamViewShader;


class NodeParamViewTextEdit : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewTextEdit(QWidget* parent = nullptr);

  QString text() const
  {
    return line_edit_->toPlainText();
  }

  // let this instance perform as a code editor.
  // This input will be edited with a built-in or external code editor.
  // 'is_vertex' is false for Fragment shader, true for vertex shader
  void setShaderCodeEditorFlag(const Node *owner, bool is_vertex);
  // set flag to view text as code issues
  void setShaderIssuesFlag();

  void SetEditInViewerOnlyMode(bool on);

public slots:
  void setText(const QString &s)
  {
    line_edit_->blockSignals(true);
    line_edit_->setPlainText(s);
    line_edit_->blockSignals(false);
  }

  void setTextPreservingCursor(const QString &s)
  {
    // Save cursor position
    int cursor_pos = line_edit_->textCursor().position();

    // Set text
    this->setText(s);

    // Get new text cursor
    QTextCursor c = line_edit_->textCursor();
    c.setPosition(cursor_pos);
    line_edit_->setTextCursor(c);
  }

signals:
  void textEdited(const QString &);

  void RequestEditInViewer();

private:
  QPlainTextEdit* line_edit_;
  bool code_editor_flag_;
  bool code_issues_flag_;
  NodeParamViewShader * shader_edit_;

private:
  QPushButton* edit_btn_;

  QPushButton *edit_in_viewer_btn_;

private slots:
  void ShowTextDialog();

  void InnerWidgetTextChanged();

  void OnTextChangedExternally(const QString & content);

};

}


#endif // NODEPARAMVIEWTEXTEDIT_H
