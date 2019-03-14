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

#include <QDebug>
#include <QMenu>

#include "dialogs/texteditdialog.h"
#include "mainwindow.h"

TextEditEx::TextEditEx(QWidget *parent, bool enable_rich_text) :
  QTextEdit(parent),
  enable_rich_text_(enable_rich_text)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(text_edit_menu()));

  connect(this, SIGNAL(textChanged()), this, SLOT(queue_text_modified()));
}

void TextEditEx::text_edit_menu() {
  QMenu menu;

  menu.addAction(tr("&Edit Text"), this, SLOT(open_text_edit()));

  menu.exec(QCursor::pos());
}

void TextEditEx::open_text_edit() {
  const QString& current_text = (enable_rich_text_) ? toHtml() : toPlainText();

  TextEditDialog ted(olive::MainWindow, current_text, enable_rich_text_);
  ted.exec();
  QString result = ted.get_string();
  if (!result.isEmpty()) {
    if (enable_rich_text_) {
      setHtml(result);
    } else {
      setPlainText(result);
    }
  }
}

void TextEditEx::queue_text_modified()
{
  emit textModified(enable_rich_text_ ? toHtml() : toPlainText());
}
