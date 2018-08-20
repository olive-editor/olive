#include "texteditex.h"

#include <QDebug>

TextEditEx::TextEditEx(QWidget *parent) : QTextEdit(parent) {}

void TextEditEx::setPlainTextEx(const QString &text) {
	blockSignals(true);

	int pos = textCursor().position();

	setPlainText(text);

	QTextCursor newCursor(document());
	newCursor.setPosition(pos);
	setTextCursor(newCursor);

	blockSignals(false);
}
