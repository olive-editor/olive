#include "effects.h"

#include <QGridLayout>
#include <QLabel>
#include <QOpenGLTexture>
#include <QTextEdit>
#include <QPainter>
#include <QPushButton>
#include <QColorDialog>
#include <QDebug>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "project/clip.h"
#include "project/sequence.h"

TextEffect::TextEffect(Clip *c) : Effect(c), texture(NULL) {
    setup_effect(EFFECT_TYPE_VIDEO, VIDEO_TEXT_EFFECT);

    QGridLayout* ui_layout = new QGridLayout();

    ui_layout->addWidget(new QLabel("Text:"), 0, 0);
    text_val = new QTextEdit();
    ui_layout->addWidget(text_val, 0, 1);

    ui_layout->addWidget(new QLabel("Size:"), 1, 0);
    size_val = new LabelSlider();
    size_val->set_minimum_value(0);
    ui_layout->addWidget(size_val, 1, 1);

    ui_layout->addWidget(new QLabel("Color:"), 2, 0);
    set_color_button = new QPushButton();
    ui_layout->addWidget(set_color_button, 2, 1);

    ui->setLayout(ui_layout);

    container->setContents(ui);

    color = Qt::white;
    set_button_color();

    connect(text_val, SIGNAL(textChanged()), this, SLOT(update_texture()));
    connect(size_val, SIGNAL(valueChanged()), this, SLOT(update_texture()));
    connect(set_color_button, SIGNAL(clicked(bool)), this, SLOT(set_color()));

    size_val->set_value(24);
    text_val->setText("Sample Text");
}

TextEffect::~TextEffect() {
    texture->destroy();
    delete texture;
}

void TextEffect::set_button_color() {
    QPalette pal = set_color_button->palette();
    pal.setColor(QPalette::Button, color);
    set_color_button->setPalette(pal);
}

void TextEffect::set_color() {
    color = QColorDialog::getColor(color, NULL);
    set_button_color();
    update_texture();
}

void TextEffect::update_texture() {
    pixmap = QPixmap(parent_clip->sequence->width, parent_clip->sequence->height);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // set font
    QFont font = p.font();
    font.setPointSize(size_val->value());
    p.setFont(font);

    p.setPen(color);
    p.drawText(QRect(0, 0, pixmap.width(), pixmap.height()), Qt::AlignCenter | Qt::TextWordWrap, text_val->toPlainText());

    if (texture == NULL) {
        texture = new QOpenGLTexture(pixmap.toImage());
    } else {
        texture->destroy();
        texture->setData(pixmap.toImage());
    }

    // queue a repaint of the canvas
    field_changed();
}

void TextEffect::post_gl() {
    if (texture != NULL) {
        texture->bind();

        int half_width = pixmap.width()/2;
        int half_height = pixmap.height()/2;

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
    e->color = color;
    return e;
}

void TextEffect::load(QXmlStreamReader* stream) {
    const QXmlStreamAttributes& attr = stream->attributes();
    for (int i=0;i<attr.size();i++) {
        const QXmlStreamAttribute& a = attr.at(i);
        if (a.name() == "size") {
            size_val->set_value(a.value().toDouble());
        } else if (a.name() == "r") {
            color.setRed(a.value().toInt());
        } else if (a.name() == "g") {
            color.setGreen(a.value().toInt());
        } else if (a.name() == "b") {
            color.setBlue(a.value().toInt());
        }
    }
    while (!(stream->name() == "effect" && stream->isEndElement())) {
        stream->readNext();
        if (stream->name() == "text" && stream->isStartElement()) {
            text_val->setPlainText(stream->text().toString());
        }
    }
}

void TextEffect::save(QXmlStreamWriter* stream) {
    stream->writeAttribute("size", QString::number(size_val->value()));
    stream->writeAttribute("r", QString::number(color.red()));
    stream->writeAttribute("g", QString::number(color.green()));
    stream->writeAttribute("b", QString::number(color.blue()));
    stream->writeTextElement("text", text_val->toPlainText());
}
