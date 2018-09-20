#ifndef FONTCOMBOBOX_H
#define FONTCOMBOBOX_H

#include "comboboxex.h"

class FontCombobox : public ComboBoxEx {
	Q_OBJECT
public:
	FontCombobox(QWidget* parent = 0);
	const QString &getPreviousValue();
private slots:
	void updateInternals();
private:
	QString previousValue;
	QString value;
};

#endif // FONTCOMBOBOX_H
