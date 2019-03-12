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

#include "texteditdialog.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

TextEditDialog::TextEditDialog(QWidget *parent, const QString &s) :
  QDialog(parent)
{
  setWindowTitle(tr("Edit Text"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  QHBoxLayout* toolbar = new QHBoxLayout();

  QPushButton* italic_button = new QPushButton("I");
  italic_button->setCheckable(true);
  toolbar->addWidget(italic_button);

  QPushButton* underline_button = new QPushButton("U");
  underline_button->setCheckable(true);
  toolbar->addWidget(underline_button);

  toolbar->addStretch();

  layout->addLayout(toolbar);

  textEdit = new QTextEdit(this);
  textEdit->setHtml(s);
  layout->addWidget(textEdit);

  // Connect buttons to the text field
  connect(italic_button, SIGNAL(clicked(bool)), textEdit, SLOT(setFontItalic(bool)));
  connect(underline_button, SIGNAL(clicked(bool)), textEdit, SLOT(setFontUnderline(bool)));

  // Create dialog buttons at the bottom
  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(save()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(cancel()));
}

const QString& TextEditDialog::get_string() {
  return result_str;
}

void TextEditDialog::save() {
  result_str = textEdit->toHtml();
  accept();
}

void TextEditDialog::cancel() {
  reject();
}
