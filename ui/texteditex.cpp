#include "texteditex.h"

#include <QDebug>

TextEditEx::TextEditEx(QWidget *parent) : QTextEdit(parent) {
	setUndoRedoEnabled(false);
	connect(this, SIGNAL(textChanged()), this, SLOT(updateInternals()));
}

void TextEditEx::setPlainTextEx(const QString &text) {
	blockSignals(true);

	int pos = textCursor().position();

	setPlainText(text);

	QTextCursor newCursor(document());
	newCursor.setPosition(pos);
	setTextCursor(newCursor);

	updateInternals();

	blockSignals(false);
}

const QString &TextEditEx::getPreviousValue() {
	return previousText;
}

void TextEditEx::updateInternals() {
	previousText = text;
	text = toPlainText();
}
