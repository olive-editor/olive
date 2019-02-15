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
#include <QPlainTextEdit>
#include <QDialogButtonBox>

TextEditDialog::TextEditDialog(QWidget *parent, const QString &s) :
	QDialog(parent)
{
	setWindowTitle(tr("Edit Text"));

	QVBoxLayout* layout = new QVBoxLayout(this);

	textEdit = new QPlainTextEdit(this);
	textEdit->setPlainText(s);
	layout->addWidget(textEdit);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);
	connect(buttons, SIGNAL(accepted()), this, SLOT(save()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(cancel()));
}

const QString& TextEditDialog::get_string() {
	return result_str;
}

void TextEditDialog::save() {
	result_str = textEdit->toPlainText();
	accept();
}

void TextEditDialog::cancel() {
	reject();
}
