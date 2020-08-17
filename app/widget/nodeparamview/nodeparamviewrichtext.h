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

#ifndef NODEPARAMVIEWRICHTEXT_H
#define NODEPARAMVIEWRICHTEXT_H

#include <QLineEdit>
#include <QWidget>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class NodeParamViewRichText : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewRichText(QWidget* parent = nullptr);

  QString text() const
  {
    return line_edit_->text();
  }

public slots:
  void setText(const QString &s)
  {
    line_edit_->setText(s);
  }

  void setTextPreservingCursor(const QString &s)
  {
    int cursor_pos = line_edit_->cursorPosition();
    line_edit_->setText(s);
    line_edit_->setCursorPosition(cursor_pos);
  }

signals:
  void textEdited(const QString &);

private:
  QLineEdit* line_edit_;

private slots:
  void ShowRichTextDialog();

};

OLIVE_NAMESPACE_EXIT

#endif // NODEPARAMVIEWRICHTEXT_H
