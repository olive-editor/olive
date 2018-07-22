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

class FontCombobox : public ComboBoxEx {
public:
    FontCombobox(QWidget* parent = 0) : ComboBoxEx(parent) {}
protected:
    void showPopup() {
        QString current = currentText();
        clear();
        QStringList fonts = QFontDatabase().families();
        bool found = false;
        for (int i=0;i<fonts.size();i++) {
            addItem(fonts.at(i));
            if (!found && fonts.at(i) == current) {
                setCurrentIndex(i);
                found = true;
            }
        }
        QComboBox::showPopup();
    }
};

TextEffect::TextEffect(Clip *c) :
    Effect(c, EFFECT_TYPE_VIDEO, VIDEO_TEXT_EFFECT),
    texture(NULL),
    width(0),
    height(0)
{
    ui_layout->addWidget(new QLabel("Text:"), 0, 0);
    text_val = new QTextEdit();
    text_val->setUndoRedoEnabled(true);
    ui_layout->addWidget(text_val, 0, 1);

    ui_layout->addWidget(new QLabel("Font:"), 1, 0);
    set_font_combobox = new FontCombobox();
    ui_layout->addWidget(set_font_combobox, 1, 1);

    ui_layout->addWidget(new QLabel("Size:"), 2, 0);
    size_val = new LabelSlider();
    size_val->set_minimum_value(0);
    ui_layout->addWidget(size_val, 2, 1);

    ui_layout->addWidget(new QLabel("Color:"), 3, 0);
    set_color_button = new ColorButton();
    ui_layout->addWidget(set_color_button, 3, 1);

    set_font_combobox->addItem(font.family());

    connect(text_val, SIGNAL(textChanged()), this, SLOT(update_texture()));
    connect(size_val, SIGNAL(valueChanged()), this, SLOT(update_texture()));
    connect(set_color_button, SIGNAL(color_changed()), this, SLOT(update_texture()));
    connect(set_font_combobox, SIGNAL(currentTextChanged(QString)), this, SLOT(update_texture()));

    size_val->set_value(24);
    text_val->setText("Sample Text");
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
    font.setFamily(set_font_combobox->currentText());
    font.setPointSize(size_val->value());
    font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
    p.setFont(font);

    p.setPen(set_color_button->get_color());
    p.drawText(QRect(0, 0, width, height), Qt::AlignCenter | Qt::TextWordWrap, text_val->toPlainText());

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
    TextEffect* e = new TextEffect(c);
    e->text_val->setPlainText(text_val->toPlainText());
    e->size_val->set_value(size_val->value());
    e->set_color_button->set_color(set_color_button->get_color());
    return e;
}

void TextEffect::load(QXmlStreamReader* stream) {
    const QXmlStreamAttributes& attr = stream->attributes();
    for (int i=0;i<attr.size();i++) {
        const QXmlStreamAttribute& a = attr.at(i);
        if (a.name() == "size") {
            size_val->set_value(a.value().toDouble());
        } else if (a.name() == "r") {
            set_color_button->get_color().setRed(a.value().toInt());
        } else if (a.name() == "g") {
            set_color_button->get_color().setGreen(a.value().toInt());
        } else if (a.name() == "b") {
            set_color_button->get_color().setBlue(a.value().toInt());
        }
    }
    while (!(stream->name() == "effect" && stream->isEndElement())) {
        stream->readNext();
        if (stream->name() == "text" && stream->isStartElement()) {
            stream->readNext();
            text_val->setPlainText(stream->text().toString());
        }
    }
}

void TextEffect::save(QXmlStreamWriter* stream) {
    stream->writeAttribute("size", QString::number(size_val->value()));
    stream->writeAttribute("r", QString::number(set_color_button->get_color().red()));
    stream->writeAttribute("g", QString::number(set_color_button->get_color().green()));
    stream->writeAttribute("b", QString::number(set_color_button->get_color().blue()));
    stream->writeTextElement("text", text_val->toPlainText());
}
