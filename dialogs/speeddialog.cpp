#include "speeddialog.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>

#include "ui/labelslider.h"

SpeedDialog::SpeedDialog(QWidget *parent) : QDialog(parent) {
	QGridLayout* layout = new QGridLayout();
	layout->setSpacing(6);
	layout->setMargin(9);

	setLayout(layout);

	layout->addWidget(new QLabel("Speed:"), 0, 0);
	percent = new LabelSlider();
	percent->set_display_type(LABELSLIDER_PERCENT);
	layout->addWidget(percent, 0, 1);

	layout->addWidget(new QLabel("Frame Rate:"), 1, 0);
	frame_rate = new LabelSlider();
	layout->addWidget(frame_rate, 1, 1);

	layout->addWidget(new QLabel("Duration:"), 2, 0);
	duration = new LabelSlider();
	duration->set_display_type(LABELSLIDER_FRAMENUMBER);
	layout->addWidget(duration, 2, 1);

	QHBoxLayout* reverseLayout = new QHBoxLayout();
	reverse = new QCheckBox("Reverse");
	reverseLayout->addWidget(reverse);
	layout->addLayout(reverseLayout, 3, 0, 2, 1);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addWidget(buttonBox, 4, 0, 2, 1);
}
