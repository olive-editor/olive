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

#ifndef EXPORTADVANCEDVIDEODIALOG_H
#define EXPORTADVANCEDVIDEODIALOG_H

#include <QComboBox>
#include <QDialog>

#include "codec/encoder.h"
#include "widget/slider/integerslider.h"

namespace olive
{

class ExportAdvancedVideoDialog : public QDialog {
	Q_OBJECT
public:
	ExportAdvancedVideoDialog(const QList<QString> &pix_fmts,
							  QWidget *parent = nullptr);

	int threads() const
	{
		return static_cast<int>(thread_slider_->GetValue());
	}

	void set_threads(int t)
	{
		thread_slider_->SetValue(t);
	}

	QString pix_fmt() const
	{
		return pixel_format_combobox_->currentText();
	}

	void set_pix_fmt(const QString &s)
	{
		pixel_format_combobox_->setCurrentText(s);
	}

	VideoParams::ColorRange yuv_range() const
	{
		return static_cast<VideoParams::ColorRange>(
			yuv_color_range_combobox_->currentIndex());
	}

	void set_yuv_range(VideoParams::ColorRange i)
	{
		yuv_color_range_combobox_->setCurrentIndex(i);
	}

private:
	IntegerSlider *thread_slider_;

	QComboBox *pixel_format_combobox_;

	QComboBox *yuv_color_range_combobox_;
};

}

#endif // EXPORTADVANCEDVIDEODIALOG_H
