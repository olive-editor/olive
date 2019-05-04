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

#include "debugdialog.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QEvent>

#include "global/debug.h"

DebugDialog* olive::DebugDialog = nullptr;

DebugDialog::DebugDialog(QWidget *parent) : QDialog(parent) {
  QVBoxLayout* layout = new QVBoxLayout(this);

  textEdit = new QTextEdit(this);
  textEdit->setWordWrapMode(QTextOption::NoWrap);
  layout->addWidget(textEdit);

  Retranslate();
}

void DebugDialog::Retranslate()
{
  setWindowTitle(tr("Debug Log"));
}

void DebugDialog::update_log() {
  textEdit->setHtml(get_debug_str());
  textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->maximum());
}

void DebugDialog::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  } else {
    QDialog::changeEvent(e);
  }
}

void DebugDialog::showEvent(QShowEvent *) {
  update_log();
}
