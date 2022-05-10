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
#include <QWidget>

#include "common/define.h"

namespace olive {

class ExternalEditorProxy;


class NodeParamViewTextEdit : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewTextEdit(QWidget* parent = nullptr);

  QString text() const
  {
    return line_edit_->toPlainText();
  }

  // set flag to edit text with code editor
  void setCodeEditorFlag();
  // set flag to view text as code issues
  void setCodeIssuesFlag();

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

private:
  QPlainTextEdit* line_edit_;
  bool code_editor_flag_;
  bool code_issues_flag_;
  ExternalEditorProxy * ext_editor_proxy_;

private:
  void launchCodeEditor(QString & text);

private slots:
  void ShowTextDialog();

  void InnerWidgetTextChanged();

  void OnTextChangedExternally( const QString & new_text);

};

}


#endif // NODEPARAMVIEWTEXTEDIT_H
