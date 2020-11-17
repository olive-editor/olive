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

#ifndef NODEPARAMVIEWRICHTEXT_H
#define NODEPARAMVIEWRICHTEXT_H

#include <QTextEdit>
#include <QWidget>

#include "common/define.h"

namespace olive {

class NodeParamViewRichText : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewRichText(QWidget* parent = nullptr);

  QString text() const
  {
    return line_edit_->toPlainText().replace('\n', QStringLiteral("<br>"));
  }

public slots:
  void setText(QString s)
  {
    line_edit_->blockSignals(true);
    line_edit_->setPlainText(s.replace(QStringLiteral("<br>"), QStringLiteral("\n")));
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
  QTextEdit* line_edit_;

private slots:
  void ShowRichTextDialog();

  void InnerWidgetTextChanged();

};

}

#endif // NODEPARAMVIEWRICHTEXT_H
