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

	connect(static_cast<QTextEdit*>(text_val->get_ui_element()), SIGNAL(textChanged()), this, SLOT(update_texture()));
	connect(static_cast<LabelSlider*>(size_val->get_ui_element()), SIGNAL(valueChanged()), this, SLOT(update_texture()));
	connect(static_cast<ColorButton*>(set_color_button->get_ui_element()), SIGNAL(color_changed()), this, SLOT(update_texture()));
	connect(static_cast<FontCombobox*>(set_font_combobox->get_ui_element()), SIGNAL(currentTextChanged(QString)), this, SLOT(update_texture()));

	size_val->set_double_default_value(24);
	text_val->set_string_value("Sample Text");
}

TextEffect::~TextEffect() {
    destroy_texture();
}

void TextEffect::destroy_texture() {
    texture->destroy();
}

void TextEffect::update_texture() {
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
	p.drawText(QRect(0, 0, width, height), Qt::AlignCenter | Qt::TextWordWrap, text_val->get_string_value());

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
