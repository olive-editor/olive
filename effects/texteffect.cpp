#include "effects.h"

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

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/fontcombobox.h"

TextEffect::TextEffect(Clip *c) :
    Effect(c, EFFECT_TYPE_VIDEO, VIDEO_TEXT_EFFECT),
    texture(NULL),
    width(0),
    height(0)
{
	EffectRow* text_row = add_row("Text:");
	text_val = text_row->add_field(EFFECT_FIELD_STRING);

	EffectRow* font_row = add_row("Font:");
	set_font_combobox = font_row->add_field(EFFECT_FIELD_FONT);

	EffectRow* size_row = add_row("Size:");
	size_val = size_row->add_field(EFFECT_FIELD_DOUBLE);
	size_val->set_double_minimum_value(0);

	EffectRow* color_row = add_row("Color:");
	set_color_button = color_row->add_field(EFFECT_FIELD_COLOR);

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

	size_val->set_double_default_value(48);
	text_val->set_string_value("Sample Text");
	halign_field->set_combo_index(1);
	valign_field->set_combo_index(1);

	connect(text_val, SIGNAL(changed()), this, SLOT(update_texture()));
	connect(size_val, SIGNAL(changed()), this, SLOT(update_texture()));
	connect(set_color_button, SIGNAL(changed()), this, SLOT(update_texture()));
	connect(set_font_combobox, SIGNAL(changed()), this, SLOT(update_texture()));
	connect(halign_field, SIGNAL(changed()), this, SLOT(update_texture()));
	connect(valign_field, SIGNAL(changed()), this, SLOT(update_texture()));

	update_texture();
}

TextEffect::~TextEffect() {
    destroy_texture();
}

void TextEffect::refresh() {
	update_texture();
}

void TextEffect::destroy_texture() {
    texture->destroy();
}

void TextEffect::update_texture() {
	if (parent_clip->sequence != NULL) {
		if (parent_clip->sequence->width != width || parent_clip->sequence->height != height) {
			width = parent_clip->sequence->width;
			height = parent_clip->sequence->height;
			pixmap = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
		}
		pixmap.fill(Qt::transparent);

		QPainter p(&pixmap);
		p.setRenderHint(QPainter::TextAntialiasing, true);

		// set font
		font.setFamily(set_font_combobox->get_font_name());
		font.setPointSize(size_val->get_double_value());
		font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
		p.setFont(font);

		p.setPen(set_color_button->get_color_value());
		p.drawText(QRect(0, 0, width, height), halign_field->get_combo_data().toInt() | valign_field->get_combo_data().toInt() | Qt::TextWordWrap, text_val->get_string_value());

		if (texture == NULL) {
			texture = new QOpenGLTexture(pixmap);
		} else {
			destroy_texture();
			texture->setData(pixmap);
		}

		texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);

		// queue a repaint of the canvas
		field_changed();
	}
}

void TextEffect::post_gl() {
    if (texture != NULL) {
        texture->bind();

        int half_width = width/2;
        int half_height = height/2;

        glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex2f(-half_width, -half_height);
        glTexCoord2f(1.0, 0.0);
        glVertex2f(half_width, -half_height);
        glTexCoord2f(1.0, 1.0);
        glVertex2f(half_width, half_height);
        glTexCoord2f(0.0, 1.0);
        glVertex2f(-half_width, half_height);
        glEnd();

        texture->release();
    }
}

Effect* TextEffect::copy(Clip* c) {
	/*TextEffect* e = new TextEffect(c);
    e->text_val->setPlainText(text_val->toPlainText());
    e->size_val->set_value(size_val->value());
	e->set_color_button->set_color(set_color_button->get_color());
	return e;*/
	return NULL;
}
