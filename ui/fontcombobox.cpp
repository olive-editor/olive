#include "fontcombobox.h"

#include <QFontDatabase>

FontCombobox::FontCombobox(QWidget* parent) : ComboBoxEx(parent) {
	addItem(QFont().family());
}

void FontCombobox::showPopup() {
	QString current = currentText();
	clear();
	QStringList fonts = QFontDatabase().families();
	bool found = false;
	for (int i=0;i<fonts.size();i++) {
		addItem(fonts.at(i));
		if (!found && fonts.at(i) == current) {
			setCurrentIndex(i);
			found = true;
		}
	}
	QComboBox::showPopup();
}
