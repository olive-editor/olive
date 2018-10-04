#include "fontcombobox.h"

#include <QFontDatabase>

FontCombobox::FontCombobox(QWidget* parent) : ComboBoxEx(parent) {
	addItems(QFontDatabase().families());

	value = currentText();

	connect(this, SIGNAL(currentTextChanged(QString)), this, SLOT(updateInternals()));
}

const QString& FontCombobox::getPreviousValue() {
	return previousValue;
}

void FontCombobox::updateInternals() {
	previousValue = value;
	value = currentText();
}
