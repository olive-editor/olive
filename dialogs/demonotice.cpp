#include "demonotice.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

DemoNotice::DemoNotice(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle("Welcome to Olive!");
	setMaximumWidth(600);

	QVBoxLayout* vlayout = new QVBoxLayout();
	setLayout(vlayout);

	QHBoxLayout* layout = new QHBoxLayout();
	layout->setMargin(10);
	layout->setSpacing(20);

	QLabel* icon = new QLabel("<html><head/><body><p><img src=\":/icons/olive-splash.png\"/></p></body></html>");
	layout->addWidget(icon);

	QLabel* text = new QLabel("<html><head/><body><p><span style=\" font-size:14pt;\">Welcome to Olive!</span></p><p>Olive is a free open-source video editor released under the GNU GPL. If you have paid for this software, you have been scammed.</p><p>This software is currently in ALPHA which means it is unstable and very likely to crash, have bugs, and have missing features. We offer no warranty so use at your own risk. Please report any bugs or feature requests at <a href=\"https://olivevideoeditor.org/\"><span style=\" text-decoration: underline; color:#007af4;\">www.olivevideoeditor.org</span></a></p><p>Thank you for trying Olive and we hope you enjoy it!</p></body></html>");
	text->setWordWrap(true);
	layout->addWidget(text);

	vlayout->addLayout(layout);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
	buttons->setCenterButtons(true);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	vlayout->addWidget(buttons);
}
