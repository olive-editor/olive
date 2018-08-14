#ifndef FONTCOMBOBOX_H
#define FONTCOMBOBOX_H

#include "comboboxex.h"

class FontCombobox : public ComboBoxEx {
public:
	FontCombobox(QWidget* parent = 0);
protected:
	void showPopup();
};

#endif // FONTCOMBOBOX_H
