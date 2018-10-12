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
