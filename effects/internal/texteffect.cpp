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

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/fontcombobox.h"

TextEffect::TextEffect(Clip *c, const EffectMeta* em) :
	Effect(c, em)
{
	enable_superimpose = true;
	//enable_shader = true;

	text_val = add_row("Text")->add_field(EFFECT_FIELD_STRING, "text", 2);

	set_font_combobox = add_row("Font")->add_field(EFFECT_FIELD_FONT, "font", 2);

	size_val = add_row("Size")->add_field(EFFECT_FIELD_DOUBLE, "size", 2);
	size_val->set_double_minimum_value(0);

	set_color_button = add_row("Color")->add_field(EFFECT_FIELD_COLOR, "color", 2);

	EffectRow* alignment_row = add_row("Alignment");
	halign_field = alignment_row->add_field(EFFECT_FIELD_COMBO, "halign");
	halign_field->add_combo_item("Left", Qt::AlignLeft);
	halign_field->add_combo_item("Center", Qt::AlignHCenter);
	halign_field->add_combo_item("Right", Qt::AlignRight);
	halign_field->add_combo_item("Justify", Qt::AlignJustify);

	valign_field = alignment_row->add_field(EFFECT_FIELD_COMBO, "valign");
	valign_field->add_combo_item("Top", Qt::AlignTop);
	valign_field->add_combo_item("Center", Qt::AlignVCenter);
	valign_field->add_combo_item("Bottom", Qt::AlignBottom);

	word_wrap_field = add_row("Word Wrap")->add_field(EFFECT_FIELD_BOOL, "wordwrap", 2);

	outline_bool = add_row("Outline")->add_field(EFFECT_FIELD_BOOL, "outline", 2);
	outline_color = add_row("Outline Color")->add_field(EFFECT_FIELD_COLOR, "outlinecolor", 2);
	outline_width = add_row("Outline Width")->add_field(EFFECT_FIELD_DOUBLE, "outlinewidth", 2);
	outline_width->set_double_minimum_value(0);

	shadow_bool = add_row("Shadow")->add_field(EFFECT_FIELD_BOOL, "shadow", 2);
	shadow_color = add_row("Shadow Color")->add_field(EFFECT_FIELD_COLOR, "shadowcolor", 2);
	shadow_distance = add_row("Shadow Distance")->add_field(EFFECT_FIELD_DOUBLE, "shadowdistance", 2);
	shadow_distance->set_double_minimum_value(0);
	shadow_softness = add_row("Shadow Softness")->add_field(EFFECT_FIELD_DOUBLE, "shadowsoftness", 2);
	shadow_softness->set_double_minimum_value(0);
	shadow_opacity = add_row("Shadow Opacity")->add_field(EFFECT_FIELD_DOUBLE, "shadowopacity", 2);
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
	outline_width->set_double_default_value(20);

	outline_enable(false);
	shadow_enable(false);

	connect(shadow_bool, SIGNAL(toggled(bool)), this, SLOT(shadow_enable(bool)));
	connect(outline_bool, SIGNAL(toggled(bool)), this, SLOT(outline_enable(bool)));

	vertPath = "common.vert";
	fragPath = "dropshadow.frag";
}

void TextEffect::redraw(double timecode) {
	img.fill(Qt::transparent);

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
		case Qt::AlignHCenter: text_x = (width/2) - (fm.width(lines.at(i))/2); break;
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
		}

		switch (valign_field->get_combo_data(timecode).toInt()) {
		case Qt::AlignTop: text_y = (fm.height()*i)+fm.ascent(); break;
		case Qt::AlignVCenter: text_y = ((height/2) - (text_height/2) - fm.descent()) + (fm.height()*(i+1)); break;
		case Qt::AlignBottom: text_y = (height - text_height - fm.descent()) + (fm.height()*(i+1)); break;
		}

		path.addText(text_x, text_y, font, lines.at(i));
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
}

void TextEffect::shadow_enable(bool e) {
	enable_shader = e;
	close();

	shadow_color->set_enabled(e);
	shadow_distance->set_enabled(e);
	shadow_softness->set_enabled(e);
	shadow_opacity->set_enabled(e);
}

void TextEffect::outline_enable(bool e) {
	outline_color->set_enabled(e);
	outline_width->set_enabled(e);
}
