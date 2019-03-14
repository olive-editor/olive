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
#include <QIcon>

#include "ui/icons.h"

TextEditDialog::TextEditDialog(QWidget *parent, const QString &s, bool rich_text) :
  QDialog(parent),
  rich_text_(rich_text)
{
  setWindowTitle(tr("Edit Text"));

  QVBoxLayout* layout = new QVBoxLayout(this);

  if (rich_text) {
    QHBoxLayout* toolbar = new QHBoxLayout();

    // Italic Button
    italic_button = new QPushButton();
    italic_button->setIcon(QIcon(":/icons/italic.svg"));
    italic_button->setCheckable(true);
    connect(italic_button, SIGNAL(clicked(bool)), textEdit, SLOT(setFontItalic(bool)));
    toolbar->addWidget(italic_button);

    // Underline Button
    underline_button = new QPushButton();
    underline_button->setIcon(QIcon(":/icons/underline.svg"));
    underline_button->setCheckable(true);
    toolbar->addWidget(underline_button);

    // Font Name
    font_list = new QFontComboBox();
    connect(font_list, SIGNAL(setCurrentFont(const QFont&)), textEdit, SLOT(setCurrentFont(const QFont&)));
    toolbar->addWidget(font_list);


    toolbar->addStretch();

    layout->addLayout(toolbar);
  }

  // Create central text editor object
  textEdit = new QTextEdit(this);
  textEdit->setUndoRedoEnabled(true);

  if (rich_text_) {
    textEdit->setHtml(s);
  } else {
    textEdit->setPlainText(s);
  }

  layout->addWidget(textEdit);

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
  result_str = rich_text_ ? textEdit->toHtml() : textEdit->toPlainText();
  accept();
}

void TextEditDialog::cancel() {
  reject();
}

void TextEditDialog::UpdateUIFromTextCursor()
{
  italic_button->setChecked(textEdit->fontItalic());
  underline_button->setChecked(textEdit->fontUnderline());
  font_list->setCurrentText(textEdit->fontFamily());
}
