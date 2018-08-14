#include "fontcombobox.h"

#include <QFontDatabase>

FontCombobox::FontCombobox(QWidget* parent) : ComboBoxEx(parent) {
	addItems(QFontDatabase().families());
}
