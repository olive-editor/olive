#ifndef CHECKBOXEX_H
#define CHECKBOXEX_H

#include <QCheckBox>

class CheckboxEx : public QCheckBox
{
	Q_OBJECT
public:
	CheckboxEx(QWidget* parent = 0);
private slots:
	void checkbox_command();
};

#endif // CHECKBOXEX_H
