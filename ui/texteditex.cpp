/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "texteditex.h"

#include <QVBoxLayout>
#include <QDebug>

#include "dialogs/texteditdialog.h"
#include "ui/menu.h"
#include "mainwindow.h"

TextEditEx::TextEditEx(QWidget *parent, bool enable_rich_text) :
  QWidget(parent),
  enable_rich_text_(enable_rich_text)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  text_editor_ = new QTextEdit();
  connect(text_editor_, SIGNAL(textChanged()), this, SLOT(queue_text_modified()));
  layout->addWidget(text_editor_);

  QPushButton* edit_button = new QPushButton(tr("Edit Text"));
  layout->addWidget(edit_button);
  connect(edit_button, SIGNAL(clicked(bool)), this, SLOT(open_text_edit()));

  /*
  text_editor_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(text_editor_, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(text_edit_menu()));
  */
}

void TextEditEx::setUndoRedoEnabled(bool e)
{
  text_editor_->setUndoRedoEnabled(e);
}

QTextDocument *TextEditEx::document()
{
  return text_editor_->document();
}

QTextCursor TextEditEx::textCursor()
{
  return text_editor_->textCursor();
}

void TextEditEx::setTextCursor(const QTextCursor &cursor)
{
  text_editor_->setTextCursor(cursor);
}

void TextEditEx::setTextHeight(int h)
{
  text_editor_->setFixedHeight(h);
}

void TextEditEx::setHtml(const QString &text)
{
  text_editor_->setHtml(text);
}

void TextEditEx::setPlainText(const QString &text)
{
  text_editor_->setPlainText(text);
}

void TextEditEx::text_edit_menu() {
  Menu menu;

  menu.addAction(tr("&Edit Text"), this, SLOT(open_text_edit()));

  menu.exec(QCursor::pos());
}

void TextEditEx::open_text_edit() {
  const QString& current_text = (enable_rich_text_) ? text_editor_->toHtml() : text_editor_->toPlainText();

  TextEditDialog ted(olive::MainWindow, current_text, enable_rich_text_);
  ted.exec();
  QString result = ted.get_string();
  if (!result.isEmpty()) {
    if (enable_rich_text_) {
      text_editor_->setHtml(result);
    } else {
      text_editor_->setPlainText(result);
    }
  }
}

void TextEditEx::queue_text_modified()
{
  emit textModified(enable_rich_text_ ? text_editor_->toHtml() : text_editor_->toPlainText());
}
