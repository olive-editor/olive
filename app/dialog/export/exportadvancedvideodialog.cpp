/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "exportadvancedvideodialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

namespace olive
{

ExportAdvancedVideoDialog::ExportAdvancedVideoDialog(
	const QList<QString> &pix_fmts, QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Advanced"));

	QVBoxLayout *layout = new QVBoxLayout(this);

	{
		// Pixel Settings
		QGroupBox *pixel_group = new QGroupBox();
		layout->addWidget(pixel_group);
		pixel_group->setTitle(tr("Pixel"));

		QGridLayout *pixel_layout = new QGridLayout(pixel_group);

		int row = 0;

		pixel_layout->addWidget(new QLabel(tr("Pixel Format:")), row, 0);

		pixel_format_combobox_ = new QComboBox();
		pixel_format_combobox_->addItems(pix_fmts);
		pixel_layout->addWidget(pixel_format_combobox_, row, 1);

		row++;

		pixel_layout->addWidget(new QLabel(tr("YUV Color Range:")), row, 0);

		yuv_color_range_combobox_ = new QComboBox();
		yuv_color_range_combobox_->addItems(
			{ tr("Limited (16-235)"), tr("Full (0-255)") });
		pixel_layout->addWidget(yuv_color_range_combobox_, row, 1);
	}

	{
		// Performance Settings
		QGroupBox *performance_group = new QGroupBox();
		layout->addWidget(performance_group);
		performance_group->setTitle(tr("Performance"));

		QGridLayout *performance_layout = new QGridLayout(performance_group);

		int row = 0;

		performance_layout->addWidget(new QLabel(tr("Threads:")), row, 0);

		thread_slider_ = new IntegerSlider();
		thread_slider_->SetMinimum(0);
		thread_slider_->SetDefaultValue(0);
		thread_slider_->InsertLabelSubstitution(0, tr("Auto"));
		performance_layout->addWidget(thread_slider_, row, 1);

		row++;
	}

	QDialogButtonBox *buttons =
		new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, &QDialogButtonBox::accepted, this,
			&ExportAdvancedVideoDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this,
			&ExportAdvancedVideoDialog::reject);
	layout->addWidget(buttons);
}

}
