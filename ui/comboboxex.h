#ifndef COMBOBOXEX_H
#define COMBOBOXEX_H

#include <QComboBox>
#include <QDebug>

class ComboBoxEx : public QComboBox {
    Q_OBJECT
public:
    ComboBoxEx(QWidget* parent = 0);
	void setCurrentIndexEx(int i);
	void setCurrentTextEx(const QString &text);
	int getPreviousIndex();
private slots:
	void index_changed(int);
private:
	int index;
	int previousIndex;
    void wheelEvent(QWheelEvent* e);
};

#endif // COMBOBOXEX_H
