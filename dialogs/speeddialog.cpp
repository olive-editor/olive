#include "speeddialog.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDebug>

#include "ui/labelslider.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "io/media.h"
#include "playback/playback.h"

SpeedDialog::SpeedDialog(QWidget *parent) : QDialog(parent) {
	QVBoxLayout* main_layout = new QVBoxLayout();
	setLayout(main_layout);

	QGridLayout* grid = new QGridLayout();
	grid->setSpacing(6);

	grid->addWidget(new QLabel("Speed:"), 0, 0);
	percent = new LabelSlider();
	percent->decimal_places = 2;
	percent->set_display_type(LABELSLIDER_PERCENT);
	grid->addWidget(percent, 0, 1);

	grid->addWidget(new QLabel("Frame Rate:"), 1, 0);
	frame_rate = new LabelSlider();
	frame_rate->decimal_places = 3;
	grid->addWidget(frame_rate, 1, 1);

	grid->addWidget(new QLabel("Duration:"), 2, 0);
	duration = new LabelSlider();
	duration->set_display_type(LABELSLIDER_FRAMENUMBER);
	grid->addWidget(duration, 2, 1);

	main_layout->addLayout(grid);

	reverse = new QCheckBox("Reverse");
	main_layout->addWidget(reverse);
	main_layout->addWidget(new QCheckBox("Maintain Audio Pitch"));
	main_layout->addWidget(new QCheckBox("Ripple"));

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->setCenterButtons(true);
	main_layout->addWidget(buttonBox);
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

	connect(percent, SIGNAL(valueChanged()), this, SLOT(percent_update()));
	connect(frame_rate, SIGNAL(valueChanged()), this, SLOT(frame_rate_update()));
	connect(duration, SIGNAL(valueChanged()), this, SLOT(duration_update()));
}

void SpeedDialog::run() {
	if (clips.size() > 1) {
		qDebug() << "[WARNING] Setting multiple clip speeds at once is unsupported and may break things - sorry!";
	}

	normal_fr = 1;

	Clip* first_clip = clips.at(0);

	switch (first_clip->media_type) {
	case MEDIA_TYPE_FOOTAGE:
		normal_fr = static_cast<Media*>(first_clip->media)->get_stream_from_file_index(true, first_clip->media_stream)->video_frame_rate;
		break;
	case MEDIA_TYPE_SEQUENCE:
		normal_fr = static_cast<Sequence*>(first_clip->media)->frame_rate;
		break;
	}

	frame_rate->set_default_value(first_clip->frame_rate);
	percent->set_default_value((first_clip->frame_rate / normal_fr) * 100);
	duration->set_default_value(first_clip->getLength());

	exec();
}

void SpeedDialog::percent_update() {
	double pc = (percent->value()*0.01);
	frame_rate->set_value(normal_fr * pc, false);
	duration->set_value(clips.at(0)->getLength() / pc, false);
}

void SpeedDialog::duration_update() {
	double pc = clips.at(0)->getLength() / duration->value();
	frame_rate->set_value(normal_fr * pc, false);
	percent->set_value(pc * 100, false);
}

void SpeedDialog::frame_rate_update() {
	double fr = (frame_rate->value());
	double pc = (fr / normal_fr);
	percent->set_value(pc * 100, false);
	duration->set_value(clips.at(0)->getLength() / pc, false);
}

void SpeedDialog::accept() {
	for (int i=0;i<clips.size();i++) {
		clips.at(i)->frame_rate = frame_rate->value();
		close_clip(clips.at(i));
	}
	QDialog::accept();
}
