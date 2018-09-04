#ifndef DEMONOTICE_H
#define DEMONOTICE_H

#include <QDialog>

namespace Ui {
class DemoNotice;
}

class DemoNotice : public QDialog
{
	Q_OBJECT

public:
	explicit DemoNotice(QWidget *parent = 0);
	~DemoNotice();

private:
	Ui::DemoNotice *ui;
};

#endif // DEMONOTICE_H
