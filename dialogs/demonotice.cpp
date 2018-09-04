#include "demonotice.h"
#include "ui_demonotice.h"

DemoNotice::DemoNotice(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DemoNotice)
{
	ui->setupUi(this);
}

DemoNotice::~DemoNotice()
{
	delete ui;
}
