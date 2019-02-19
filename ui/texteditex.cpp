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

TextEditEx::TextEditEx(QWidget *parent) : QTextEdit(parent) {
	setUndoRedoEnabled(false);
	connect(this, SIGNAL(textChanged()), this, SLOT(updateInternals()));
	connect(this, SIGNAL(updateSelf()), this, SLOT(updateText()));
}

const QString& TextEditEx::getPlainTextEx() {
	return text;
}

void TextEditEx::setPlainTextEx(const QString &t) {
	previousText = text;
	text = t;
	emit updateSelf();
}

const QString &TextEditEx::getPreviousValue() {
	return previousText;
}

void TextEditEx::updateInternals() {
	previousText = text;
	text = toPlainText();
}

void TextEditEx::updateText() {
	blockSignals(true);

	int pos = textCursor().position();

	setPlainText(text);

	QTextCursor newCursor(document());
	newCursor.setPosition(pos);
	setTextCursor(newCursor);

	blockSignals(false);
}
