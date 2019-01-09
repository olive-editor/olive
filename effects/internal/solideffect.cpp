/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
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
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "solideffect.h"

#include <QOpenGLTexture>
#include <QPainter>
#include <QtMath>
#include <QVariant>
#include <QImage>
#include <QComboBox>

#include "project/clip.h"
#include "project/sequence.h"

#define SOLID_TYPE_COLOR 0
#define SOLID_TYPE_BARS 1
#define SOLID_TYPE_CHECKERBOARD 2

#define SMPTE_BARS 7
#define SMPTE_STRIP_COUNT 3
#define SMPTE_LOWER_BARS 4

SolidEffect::SolidEffect(Clip* c, const EffectMeta* em) : Effect(c, em) {
	enable_superimpose = true;

	solid_type = add_row("Type")->add_field(EFFECT_FIELD_COMBO, "type");
	solid_type->add_combo_item("Solid Color", SOLID_TYPE_COLOR);
	solid_type->add_combo_item("SMPTE Bars", SOLID_TYPE_BARS);
	solid_type->add_combo_item("Checkerboard", SOLID_TYPE_CHECKERBOARD);

	opacity_field = add_row("Opacity")->add_field(EFFECT_FIELD_DOUBLE, "opacity");
	opacity_field->set_double_minimum_value(0);
	opacity_field->set_double_maximum_value(100);
	opacity_field->set_double_default_value(100);

	solid_color_field = add_row("Color")->add_field(EFFECT_FIELD_COLOR, "color");
	solid_color_field->set_color_value(Qt::red);

	checkerboard_size_field = add_row("Checkerboard Size")->add_field(EFFECT_FIELD_DOUBLE, "checker_size");
	checkerboard_size_field->set_double_minimum_value(1);
	checkerboard_size_field->set_double_default_value(10);

	// hacky but eh
	QComboBox* solid_type_combo = static_cast<QComboBox*>(solid_type->get_ui_element());
	connect(solid_type_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(ui_update(int)));
	ui_update(solid_type_combo->currentIndex());

	/*vertPath = ":/shaders/common.vert";
	fragPath = ":/shaders/solideffect.frag";*/
}

void SolidEffect::redraw(double timecode) {
	int w = img.width();
	int h = img.height();
	int alpha = qRound(opacity_field->get_double_value(timecode)*2.55);
	switch (solid_type->get_combo_data(timecode).toInt()) {
	case SOLID_TYPE_COLOR:
	{
		QColor solidColor = solid_color_field->get_color_value(timecode);
		solidColor.setAlpha(alpha);
		img.fill(solidColor);
	}
		break;
	case SOLID_TYPE_BARS:
	{
		// draw smpte bars
		QPainter p(&img);
		img.fill(Qt::transparent);
		int bar_width = qCeil((double) w / 7.0);
		int first_bar_height = qCeil((double) h / 3.0 * 2.0);
		int second_bar_height = qCeil((double) h / 12.5);
		int third_bar_y = first_bar_height + second_bar_height;
		int third_bar_height = h - third_bar_y;
		int third_bar_width = 0;
		int bar_x, strip_width;
		QColor first_color, second_color, third_color;
		for (int i=0;i<SMPTE_BARS;i++) {
			bar_x = bar_width*i;
			switch (i) {
			case 0:
				first_color = QColor(192, 192, 192);
				second_color = QColor(0, 0, 192);
				break;
			case 1:
				first_color = QColor(192, 192, 0);
				second_color = QColor(19, 19, 19);
				break;
			case 2:
				first_color = QColor(0, 192, 192);
				second_color = QColor(192, 0, 192);
				break;
			case 3:
				first_color = QColor(0, 192, 0);
				second_color = QColor(19, 19, 19);
				break;
			case 4:
				first_color = QColor(192, 0, 192);
				second_color = QColor(0, 192, 192);
				break;
			case 5:
				third_bar_width = qRound((double) bar_x / (double) SMPTE_LOWER_BARS);

				first_color = QColor(192, 0, 0);
				second_color = QColor(19, 19, 19);

				strip_width = qCeil(bar_width/SMPTE_STRIP_COUNT);
				for (int j=0;j<SMPTE_STRIP_COUNT;j++) {
					switch (j) {
					case 0:
						third_color = QColor(29, 29, 29, alpha);
						p.fillRect(QRect(bar_x, third_bar_y, bar_width, third_bar_height), third_color);
						break;
					case 1:
						third_color = QColor(19, 19, 19, alpha);
						p.fillRect(QRect(bar_x + strip_width, third_bar_y, strip_width, third_bar_height), third_color);
						break;
					case 2:
						third_color = QColor(9, 9, 9, alpha);
						p.fillRect(QRect(bar_x, third_bar_y, strip_width, third_bar_height), third_color);
						break;
					}
				}
				break;
			case 6:
				first_color = QColor(0, 0, 192);
				second_color = QColor(192, 192, 192);
				p.fillRect(QRect(bar_x, third_bar_y, bar_width, third_bar_height), QColor(19, 19, 19, alpha));
				break;
			}
			first_color.setAlpha(alpha);
			second_color.setAlpha(alpha);
			p.fillRect(QRect(bar_x, 0, bar_width, first_bar_height), first_color);
			p.fillRect(QRect(bar_x, first_bar_height, bar_width, second_bar_height), second_color);
		}
		for (int i=0;i<SMPTE_LOWER_BARS;i++) {
			bar_x = third_bar_width*i;
			switch (i) {
			case 0: third_color = QColor(0, 33, 76); break;
			case 1: third_color = QColor(255, 255, 255); break;
			case 2: third_color = QColor(50, 0, 106); break;
			case 3: third_color = QColor(19, 19, 19); break;
			}
			third_color.setAlpha(alpha);
			p.fillRect(QRect(bar_x, third_bar_y, third_bar_width, third_bar_height), third_color);
		}
	}
		break;
	case SOLID_TYPE_CHECKERBOARD:
	{
		// draw checkboard
		QPainter p(&img);
		img.fill(Qt::transparent);

		int checker_width = qCeil(checkerboard_size_field->get_double_value(timecode));
		int checker_x, checker_y;
		int checkerboard_size_w = qCeil(double(w)/checker_width);
		int checkerboard_size_h = qCeil(double(h)/checker_width);

		QColor checker_odd(QColor(0, 0, 0, alpha));
		QColor checker_even(solid_color_field->get_color_value(timecode));
		checker_even.setAlpha(alpha);
		QVector<QColor> checker_color{checker_odd, checker_even};

		for(int i = 0; i < checkerboard_size_w; i++){
			checker_x = checker_width*i;
			for(int j = 0; j < checkerboard_size_h; j++){
				checker_y = checker_width*j;
				p.fillRect(QRect(checker_x, checker_y, checker_width, checker_width), checker_color[(i + j)%2]);
			}
		}
	}
		break;
	}
}

void SolidEffect::ui_update(int i) {
	solid_color_field->set_enabled(i == SOLID_TYPE_COLOR || i == SOLID_TYPE_CHECKERBOARD);
	checkerboard_size_field->set_enabled(i == SOLID_TYPE_CHECKERBOARD);
}
