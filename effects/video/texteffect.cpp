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
#include <QDebug>
#include <QWidget>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/fontcombobox.h"

TextEffect::TextEffect(Clip *c) :
	Effect(c, EFFECT_TYPE_VIDEO, VIDEO_TEXT_EFFECT)
{
	enable_image = true;

	text_val = add_row("Text:")->add_field(EFFECT_FIELD_STRING, 2);

	set_font_combobox = add_row("Font:")->add_field(EFFECT_FIELD_FONT, 2);

	size_val = add_row("Size:")->add_field(EFFECT_FIELD_DOUBLE, 2);
	size_val->set_double_minimum_value(0);

	set_color_button = add_row("Color:")->add_field(EFFECT_FIELD_COLOR, 2);

	EffectRow* alignment_row = add_row("Alignment:");
	halign_field = alignment_row->add_field(EFFECT_FIELD_COMBO);
	halign_field->add_combo_item("Left", Qt::AlignLeft);
	halign_field->add_combo_item("Center", Qt::AlignHCenter);
	halign_field->add_combo_item("Right", Qt::AlignRight);
	halign_field->add_combo_item("Justify", Qt::AlignJustify);
	valign_field = alignment_row->add_field(EFFECT_FIELD_COMBO);
	valign_field->add_combo_item("Top", Qt::AlignTop);
	valign_field->add_combo_item("Center", Qt::AlignVCenter);
	valign_field->add_combo_item("Bottom", Qt::AlignBottom);

	word_wrap_field = add_row("Word Wrap:")->add_field(EFFECT_FIELD_BOOL, 2);

	outline_bool = add_row("Outline:")->add_field(EFFECT_FIELD_BOOL, 2);
	outline_color = add_row("Outline Color:")->add_field(EFFECT_FIELD_COLOR, 2);
	outline_width = add_row("Outline Width:")->add_field(EFFECT_FIELD_DOUBLE, 2);
	outline_width->set_double_minimum_value(0);

	shadow_bool = add_row("Shadow:")->add_field(EFFECT_FIELD_BOOL, 2);
	shadow_color = add_row("Shadow Color:")->add_field(EFFECT_FIELD_COLOR, 2);
	shadow_distance = add_row("Shadow Distance:")->add_field(EFFECT_FIELD_DOUBLE, 2);
	shadow_distance->set_double_minimum_value(0);
	shadow_softness = add_row("Shadow Softness:")->add_field(EFFECT_FIELD_DOUBLE, 2);
	shadow_softness->set_double_minimum_value(0);
	shadow_opacity = add_row("Shadow Opacity:")->add_field(EFFECT_FIELD_DOUBLE, 2);
	shadow_opacity->set_double_minimum_value(0);
	shadow_opacity->set_double_maximum_value(100);

	size_val->set_double_default_value(48);
	text_val->set_string_value("Sample Text");
	halign_field->set_combo_index(1);
	valign_field->set_combo_index(1);
	word_wrap_field->set_bool_value(true);
	outline_color->set_color_value(Qt::black);
	shadow_color->set_color_value(Qt::black);
	shadow_opacity->set_double_default_value(100);
	shadow_softness->set_double_default_value(5);
	shadow_distance->set_double_default_value(5);
	shadow_opacity->set_double_default_value(80);
	outline_width->set_double_default_value(2);

	outline_enable(false);
	shadow_enable(false);

	connect(text_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(size_val, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(set_color_button, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(set_font_combobox, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(halign_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(valign_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(word_wrap_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(outline_bool, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(outline_color, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(outline_width, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(shadow_bool, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(shadow_color, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(shadow_distance, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(shadow_softness, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(shadow_opacity, SIGNAL(changed()), this, SLOT(field_changed()));

	connect(shadow_bool, SIGNAL(toggled(bool)), this, SLOT(shadow_enable(bool)));
	connect(outline_bool, SIGNAL(toggled(bool)), this, SLOT(outline_enable(bool)));
}

void TextEffect::shadow_enable(bool e) {
	shadow_color->set_enabled(e);
	shadow_distance->set_enabled(e);
	shadow_softness->set_enabled(e);
	shadow_opacity->set_enabled(e);
}

void TextEffect::outline_enable(bool e) {
	outline_color->set_enabled(e);
	outline_width->set_enabled(e);
}

QImage blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly = false)
{
	int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
	int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

	QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
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

	return result;
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

void TextEffect::process_image(long frame, QImage& img) {
	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing);
	int width = img.width();
	int height = img.height();

	// set font
	font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
	font.setFamily(set_font_combobox->get_font_name(frame));
	font.setPointSize(size_val->get_double_value(frame));
	p.setFont(font);
	QFontMetrics fm(font);

	QStringList lines = text_val->get_string_value(frame).split('\n');

	// word wrap function
	if (word_wrap_field->get_bool_value(frame)) {
		for (int i=0;i<lines.size();i++) {
			const QString& s = lines.at(i);
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

		switch (halign_field->get_combo_data(frame).toInt()) {
		case Qt::AlignLeft: text_x = 0; break;
		case Qt::AlignHCenter: text_x = (width/2) - (fm.width(lines.at(i))/2); break;
		case Qt::AlignRight: text_x = width - fm.width(lines.at(i)); break;
		case Qt::AlignJustify:
			// add spaces until the string is too big
			text_x = 0;
			while (fm.width(lines.at(i) < width)) {
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
		}

		switch (valign_field->get_combo_data(frame).toInt()) {
		case Qt::AlignTop: text_y = (fm.height()*i)+fm.ascent(); break;
		case Qt::AlignVCenter: text_y = ((height/2) - (text_height/2) - fm.descent()) + (fm.height()*(i+1)); break;
		case Qt::AlignBottom: text_y = (height - text_height - fm.descent()) + (fm.height()*(i+1)); break;
		}

		path.addText(text_x, text_y, font, lines.at(i));
	}

	p.setPen(Qt::NoPen);

	// draw shadow
	if (shadow_bool->get_bool_value(frame)) {
		QImage shadow(width, height, QImage::Format_ARGB32_Premultiplied);
		shadow.fill(Qt::transparent);
		QPainter spaint(&shadow);

		int shadow_offset = shadow_distance->get_double_value(frame);
		QColor col = shadow_color->get_color_value(frame);
		col.setAlphaF(shadow_opacity->get_double_value(frame)*0.01);
		spaint.setBrush(col);
		spaint.drawPath(path);

		blurred2(shadow, shadow.rect(), shadow_softness->get_double_value(frame), false);
		p.drawImage(shadow_offset, shadow_offset, shadow);

		spaint.end();
	}

	// draw outline
	int outline_width_val = outline_width->get_double_value(frame);
	if (outline_bool->get_bool_value(frame) && outline_width_val > 0) {
		QPen outline(outline_color->get_color_value(frame));
		outline.setWidth(outline_width_val);
		p.setPen(outline);
	}

	// draw "master" text
	p.setBrush(set_color_button->get_color_value(frame));
	p.drawPath(path);
}

Effect* TextEffect::copy(Clip* c) {
	TextEffect* e = new TextEffect(c);
	copy_field_keyframes(e);
	return e;
}
