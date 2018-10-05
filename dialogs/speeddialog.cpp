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
#include "panels/panels.h"
#include "panels/timeline.h"

SpeedDialog::SpeedDialog(QWidget *parent) : QDialog(parent) {
	QVBoxLayout* main_layout = new QVBoxLayout();
	setLayout(main_layout);

	QGridLayout* grid = new QGridLayout();
	grid->setSpacing(6);

	grid->addWidget(new QLabel("Speed:"), 0, 0);
	percent = new LabelSlider();
	percent->decimal_places = 2;
	percent->set_display_type(LABELSLIDER_PERCENT);
	percent->set_default_value(1);
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
	maintain_pitch = new QCheckBox("Maintain Audio Pitch");
	ripple = new QCheckBox("Ripple Changes");

	main_layout->addWidget(reverse);
	main_layout->addWidget(maintain_pitch);
	main_layout->addWidget(ripple);

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
	bool enable_frame_rate = false;

	default_frame_rate = qSNaN();
	current_frame_rate = qSNaN();
	current_percent = qSNaN();
	default_length = -1;
	current_length = -1;

	for (int i=0;i<clips.size();i++) {
		Clip* c = clips.at(i);

		double clip_percent;

		// get default frame rate/percentage
		clip_percent = c->speed;
		if (c->track < 0) {
			bool process_video = true;
			if (c->media_type == MEDIA_TYPE_FOOTAGE) {
				Media* m = static_cast<Media*>(c->media);
				if (m != NULL) {
					MediaStream* ms = m->get_stream_from_file_index(true, c->media_stream);
					if (ms != NULL && ms->infinite_length) {
						process_video = false;
					}
				}
			}

			if (process_video) {
				double media_frame_rate = c->getMediaFrameRate();

				// get "default" frame rate"
				if (enable_frame_rate) {
					// check if frame rate is equal to default
					if (!qIsNaN(default_frame_rate) && !qFuzzyCompare(media_frame_rate, default_frame_rate)) {
						default_frame_rate = qSNaN();
					}
					if (!qIsNaN(current_frame_rate) && !qFuzzyCompare(media_frame_rate*c->speed, current_frame_rate)) {
						current_frame_rate = qSNaN();
					}
				} else {
					default_frame_rate = media_frame_rate;
					current_frame_rate = media_frame_rate*c->speed;
				}

				enable_frame_rate = true;
			}
		}

		// get default length
		long clip_default_length = qRound(c->getLength() * clip_percent);
		if (i == 0) {
			current_length = c->getLength();
			default_length = clip_default_length;
			current_percent = clip_percent;
		} else {
			if (current_length != -1 && c->getLength() != current_length) {
				current_length = -1;
			}
			if (default_length != -1 && clip_default_length != default_length) {
				default_length = -1;
			}
			if (!qIsNaN(current_percent) && !qFuzzyCompare(clip_percent, current_percent)) {
				current_percent = qSNaN();
			}
		}
	}

	frame_rate->setEnabled(enable_frame_rate);
	frame_rate->set_default_value(default_frame_rate);
	frame_rate->set_value(current_frame_rate, false);
	percent->set_value(current_percent, false);
	duration->set_default_value(default_length);
	duration->set_value(current_length, false);

	exec();
}

void SpeedDialog::percent_update() {
	frame_rate->set_value(default_frame_rate * percent->value(), false);
	duration->set_value(default_length / percent->value(), false);
}

void SpeedDialog::duration_update() {
	double pc = default_length / duration->value();
	frame_rate->set_value(default_frame_rate * pc, false);
	percent->set_value(pc, false);
}

void SpeedDialog::frame_rate_update() {
	double fr = (frame_rate->value());
	double pc = (fr / default_frame_rate);
	percent->set_value(pc, false);
	duration->set_value(default_length / pc, false);
}

void SpeedDialog::accept() {
	// TODO make undoable lmao
	for (int i=0;i<clips.size();i++) {
		Clip* c = clips.at(i);
		close_clip(c);
		long proposed_out = c->timeline_out;
		if (!qIsNaN(percent->value())) {
			double multiplier = (c->speed / percent->value());
			proposed_out = c->timeline_in + (c->getLength() * multiplier);
			c->clip_in *= multiplier;
			c->speed = percent->value();
		}
		if (proposed_out > c->timeline_out) {
			for (int i=0;i<c->sequence->clips.size();i++) {
				Clip* compare = c->sequence->clips.at(i);
				if (compare != NULL
						&& compare->track == c->track
						&& compare->timeline_in >= c->timeline_out && compare->timeline_in < proposed_out) {
					proposed_out = compare->timeline_in;
				}
			}
		}
		c->timeline_out = proposed_out;

		c->recalculateMaxLength();

		c->maintain_audio_pitch = maintain_pitch->isChecked();
	}
	panel_timeline->redraw_all_clips(true);
	QDialog::accept();
}
