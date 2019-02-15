/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "stabilizerdialog.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>

#include "ui/labelslider.h"

// NOTE: this is never used in Olive, hasn't been maintained, and needs to be written

StabilizerDialog::StabilizerDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Stabilizer");

	layout = new QVBoxLayout(this);

	enable_stab = new QCheckBox(this);
	enable_stab->setText("Enable Stabilizer");
	layout->addWidget(enable_stab);

	analysis = new QGroupBox("Analysis", this);
	layout->addWidget(analysis);

	analysis_layout = new QGridLayout(analysis);
	analysis->setLayout(analysis_layout);

	analysis_layout->addWidget(new QLabel("Shakiness:", this), 0, 0);

	shakiness_slider = new LabelSlider(this);
	shakiness_slider->set_minimum_value(1);
	shakiness_slider->set_default_value(5);
	shakiness_slider->set_maximum_value(10);
	analysis_layout->addWidget(shakiness_slider, 0, 1);

	analysis_layout->addWidget(new QLabel("Accuracy:", this), 1, 0);

	accuracy_slider = new LabelSlider(this);
	accuracy_slider->set_minimum_value(1);
	accuracy_slider->set_default_value(15);
	accuracy_slider->set_maximum_value(15);
	analysis_layout->addWidget(accuracy_slider, 1, 1);

	analysis_layout->addWidget(new QLabel("Step Size:", this), 2, 0);

	stepsize_slider = new LabelSlider(this);
	stepsize_slider->set_minimum_value(1);
	stepsize_slider->set_default_value(6);
	analysis_layout->addWidget(stepsize_slider, 2, 1);

	analysis_layout->addWidget(new QLabel("Minimum Contrast:", this), 3, 0);

	mincontrast_slider = new LabelSlider(this);
	mincontrast_slider->set_minimum_value(0);
	mincontrast_slider->set_default_value(0.3);
	mincontrast_slider->set_maximum_value(1);
	analysis_layout->addWidget(mincontrast_slider, 3, 1);

	/*analysis_layout->addWidget(new QLabel("Tripod Mode:"), 4, 0);

	tripod_mode_box = new QCheckBox();
	analysis_layout->addWidget(tripod_mode_box, 4, 1);*/

	stabilization = new QGroupBox("Stabilization", this);
	layout->addWidget(stabilization);

	stabilization_layout = new QGridLayout(this);
	stabilization->setLayout(stabilization_layout);

	stabilization_layout->addWidget(new QLabel("Smoothing:", this), 0, 0);

	smoothing_slider = new LabelSlider();
	smoothing_slider->set_minimum_value(0);
	smoothing_slider->set_default_value(10);
	stabilization_layout->addWidget(smoothing_slider, 0, 1);

	stabilization_layout->addWidget(new QLabel("Gaussian Motion:"), 1, 0);

	gaussian_motion = new QCheckBox();
	gaussian_motion->setChecked(true);
	stabilization_layout->addWidget(gaussian_motion, 1, 1);

	stabilization_layout->addWidget(new QLabel("Maximum Movement:"), 2, 0);
	stabilization_layout->addWidget(new QLabel("Maximum Rotation:"), 3, 0);
	stabilization_layout->addWidget(new QLabel("Crop:"), 4, 0);
	stabilization_layout->addWidget(new QLabel("Zoom Behavior:"), 5, 0);
	stabilization_layout->addWidget(new QLabel("Zoom Speed:"), 6, 0);
	stabilization_layout->addWidget(new QLabel("Interpolation Quality:"), 7, 0);

	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);

	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	connect(enable_stab, SIGNAL(toggled(bool)), this, SLOT(set_all_enabled(bool)));

	set_all_enabled(false);
}

void StabilizerDialog::set_all_enabled(bool e) {
	analysis->setEnabled(e);
	stabilization->setEnabled(e);
}
