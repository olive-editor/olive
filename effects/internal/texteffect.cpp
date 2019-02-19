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

#include "texteffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QOpenGLTexture>
#include <QTextEdit>
#include <QPainter>
#include <QPushButton>
#include <QColorDialog>
#include <QFontDatabase>
#include <QComboBox>
#include <QWidget>
#include <QtMath>
#include <QMenu>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/fontcombobox.h"
#include "dialogs/texteditdialog.h"
#include "io/config.h"
#include "mainwindow.h"

TextEffect::TextEffect(Clip *c, const EffectMeta* em) :
	Effect(c, em)
{
	enable_superimpose = true;

	text_val = add_row(tr("Text"))->add_field(EFFECT_FIELD_STRING, "text", 2);
	QTextEdit* text_widget = static_cast<QTextEdit*>(text_val->ui_element);
	text_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(text_widget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(text_edit_menu()));

	set_font_combobox = add_row(tr("Font"))->add_field(EFFECT_FIELD_FONT, "font", 2);

	size_val = add_row(tr("Size"))->add_field(EFFECT_FIELD_DOUBLE, "size", 2);
	size_val->set_double_minimum_value(0);

	set_color_button = add_row(tr("Color"))->add_field(EFFECT_FIELD_COLOR, "color", 2);

	EffectRow* alignment_row = add_row(tr("Alignment"));
	halign_field = alignment_row->add_field(EFFECT_FIELD_COMBO, "halign");
	halign_field->add_combo_item(tr("Left"), Qt::AlignLeft);
	halign_field->add_combo_item(tr("Center"), Qt::AlignHCenter);
	halign_field->add_combo_item(tr("Right"), Qt::AlignRight);
	halign_field->add_combo_item(tr("Justify"), Qt::AlignJustify);

	valign_field = alignment_row->add_field(EFFECT_FIELD_COMBO, "valign");
	valign_field->add_combo_item(tr("Top"), Qt::AlignTop);
	valign_field->add_combo_item(tr("Center"), Qt::AlignVCenter);
	valign_field->add_combo_item(tr("Bottom"), Qt::AlignBottom);

	word_wrap_field = add_row(tr("Word Wrap"))->add_field(EFFECT_FIELD_BOOL, "wordwrap", 2);

	outline_bool = add_row(tr("Outline"))->add_field(EFFECT_FIELD_BOOL, "outline", 2);
	outline_color = add_row(tr("Outline Color"))->add_field(EFFECT_FIELD_COLOR, "outlinecolor", 2);
	outline_width = add_row(tr("Outline Width"))->add_field(EFFECT_FIELD_DOUBLE, "outlinewidth", 2);
	outline_width->set_double_minimum_value(0);

	shadow_bool = add_row(tr("Shadow"))->add_field(EFFECT_FIELD_BOOL, "shadow", 2);
	shadow_color = add_row(tr("Shadow Color"))->add_field(EFFECT_FIELD_COLOR, "shadowcolor", 2);
	shadow_distance = add_row(tr("Shadow Distance"))->add_field(EFFECT_FIELD_DOUBLE, "shadowdistance", 2);
	shadow_distance->set_double_minimum_value(0);
	shadow_softness = add_row(tr("Shadow Softness"))->add_field(EFFECT_FIELD_DOUBLE, "shadowsoftness", 2);
	shadow_softness->set_double_minimum_value(0);
	shadow_opacity = add_row(tr("Shadow Opacity"))->add_field(EFFECT_FIELD_DOUBLE, "shadowopacity", 2);
	shadow_opacity->set_double_minimum_value(0);
	shadow_opacity->set_double_maximum_value(100);

	size_val->set_double_default_value(48);
	text_val->set_string_value(tr("Sample Text"));
	halign_field->set_combo_index(1);
	valign_field->set_combo_index(1);
	word_wrap_field->set_bool_value(true);
	outline_color->set_color_value(Qt::black);
	shadow_color->set_color_value(Qt::black);
	shadow_opacity->set_double_default_value(100);
	shadow_softness->set_double_default_value(5);
	shadow_distance->set_double_default_value(5);
	shadow_opacity->set_double_default_value(80);
	outline_width->set_double_default_value(20);

	outline_enable(false);
	shadow_enable(false);

	connect(shadow_bool, SIGNAL(toggled(bool)), this, SLOT(shadow_enable(bool)));
	connect(outline_bool, SIGNAL(toggled(bool)), this, SLOT(outline_enable(bool)));

	vertPath = "common.vert";
	fragPath = "dropshadow.frag";
}

void blurred2(QImage& result, const QRect& rect, int radius, bool alphaOnly = false) {
	int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
	int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

	int r1 = rect.top();
	int r2 = rect.bottom();
	int c1 = rect.left();
	int c2 = rect.right();

	int bpl = result.bytesPerLine();
	int rgba[4];
	unsigned char* p;

	int i1 = 0;
	int i2 = 3;

	if (alphaOnly)
		i1 = i2 = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

	for (int col = c1; col <= c2; col++) {
		p = result.scanLine(r1) + col * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p += bpl;
		for (int j = r1; j < r2; j++, p += bpl)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}

	for (int row = r1; row <= r2; row++) {
		p = result.scanLine(row) + c1 * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p += 4;
		for (int j = c1; j < c2; j++, p += 4)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}

	for (int col = c1; col <= c2; col++) {
		p = result.scanLine(r2) + col * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p -= bpl;
		for (int j = r1; j < r2; j++, p -= bpl)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}

	for (int row = r1; row <= r2; row++) {
		p = result.scanLine(row) + c2 * 4;
		for (int i = i1; i <= i2; i++)
			rgba[i] = p[i] << 4;

		p -= 4;
		for (int j = c1; j < c2; j++, p -= 4)
			for (int i = i1; i <= i2; i++)
				p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
	}
}

void TextEffect::redraw(double timecode) {
	QColor bkg = set_color_button->get_color_value(timecode);
	bkg.setAlpha(0);
	img.fill(bkg);

	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing);
	int width = img.width();
	int height = img.height();

	// set font
	font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
	font.setFamily(set_font_combobox->get_font_name(timecode));
	font.setPointSize(size_val->get_double_value(timecode));
	p.setFont(font);
	QFontMetrics fm(font);

	QStringList lines = text_val->get_string_value(timecode).split('\n');

	// word wrap function
	if (word_wrap_field->get_bool_value(timecode)) {
		for (int i=0;i<lines.size();i++) {
			QString s(lines.at(i));
			if (fm.width(s) > width) {
				int last_space_index = 0;
				for (int j=0;j<s.length();j++) {
					if (s.at(j) == ' ') {
						if (fm.width(s.left(j)) > width) {
							break;
						} else {
							last_space_index = j;
						}
					}
				}
				if (last_space_index > 0) {
					lines.insert(i+1, s.mid(last_space_index + 1));
					lines[i] = s.left(last_space_index);
				}
			}
		}
	}

	QPainterPath path;

	int text_height = fm.height()*lines.size();

	for (int i=0;i<lines.size();i++) {
		int text_x, text_y;

		switch (halign_field->get_combo_data(timecode).toInt()) {
		case Qt::AlignLeft: text_x = 0; break;
		case Qt::AlignRight: text_x = width - fm.width(lines.at(i)); break;
		case Qt::AlignJustify:
			// add spaces until the string is too big
			text_x = 0;
			while (fm.width(lines.at(i)) < width) {
				bool space = false;
				QString spaced(lines.at(i));
				for (int i=0;i<spaced.length();i++) {
					if (spaced.at(i) == ' ') {
						// insert a space
						spaced.insert(i, ' ');
						space = true;

						// scan to next non-space
						while (i < spaced.length() && spaced.at(i) == ' ') i++;
					}
				}
				if (fm.width(spaced) > width || !space) {
					break;
				} else {
					lines[i] = spaced;
				}
			}
			break;
		case Qt::AlignHCenter:
		default:
			text_x = (width/2) - (fm.width(lines.at(i))/2);
			break;
		}

		switch (valign_field->get_combo_data(timecode).toInt()) {
		case Qt::AlignTop:
			text_y = (fm.height()*i)+fm.ascent();
			break;
		case Qt::AlignBottom:
			text_y = (height - text_height - fm.descent()) + (fm.height()*(i+1));
			break;
		case Qt::AlignVCenter:
		default:
			text_y = ((height/2) - (text_height/2) - fm.descent()) + (fm.height()*(i+1));
			break;
		}

		path.addText(text_x, text_y, font, lines.at(i));
	}

	// draw software shadow
	if (shadow_bool->get_bool_value(timecode)) {
		p.setPen(Qt::NoPen);
		int shadow_offset = shadow_distance->get_double_value(timecode);

		QPainterPath shadow_path(path);
		shadow_path.translate(shadow_offset, shadow_offset);

		QColor col = shadow_color->get_color_value(timecode);
		col.setAlpha(0);
		img.fill(col);

		col.setAlphaF(shadow_opacity->get_double_value(timecode)*0.01);
		p.setBrush(col);
		p.drawPath(shadow_path);

		int blurSoftness = shadow_softness->get_double_value(timecode);
		if (blurSoftness > 0) blurred2(img, img.rect(), blurSoftness, false);
	}

	// draw outline
	int outline_width_val = outline_width->get_double_value(timecode);
	if (outline_bool->get_bool_value(timecode) && outline_width_val > 0) {
		QPen outline(outline_color->get_color_value(timecode));
		outline.setWidth(outline_width_val);
		p.setPen(outline);
		p.setBrush(Qt::NoBrush);
		p.drawPath(path);
	}

	// draw "master" text
	p.setPen(Qt::NoPen);
	p.setBrush(set_color_button->get_color_value(timecode));
	p.drawPath(path);

	p.end();
}

void TextEffect::shadow_enable(bool e) {
	close();

	shadow_color->set_enabled(e);
	shadow_distance->set_enabled(e);
	shadow_softness->set_enabled(e);
	shadow_opacity->set_enabled(e);
}

void TextEffect::text_edit_menu() {
	QMenu menu;

	menu.addAction(tr("&Edit Text"), this, SLOT(open_text_edit()));

	menu.exec(QCursor::pos());
}

void TextEffect::open_text_edit() {
    TextEditDialog ted(Olive::MainWindow, text_val->get_current_data().toString());
	ted.exec();
	QString result = ted.get_string();
	if (!result.isEmpty()) {
		text_val->set_current_data(result);
		text_val->ui_element_change();
	}
}

void TextEffect::outline_enable(bool e) {
	outline_color->set_enabled(e);
	outline_width->set_enabled(e);
}
