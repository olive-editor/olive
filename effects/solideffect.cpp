#include "effects.h"

#include <QOpenGLTexture>
#include <QPainter>
#include <QtMath>

#include "project/clip.h"
#include "project/sequence.h"

#define SMPTE_BARS 7
#define SMPTE_STRIP_COUNT 3
#define SMPTE_LOWER_BARS 4

bool solid = false;

SolidEffect::SolidEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_SOLID_EFFECT), texture(NULL) {
    update_texture();
}

void SolidEffect::update_texture() {
    QImage img(parent_clip->sequence->width, parent_clip->sequence->height, QImage::Format_RGB888);

    if (solid) {
        img.fill(Qt::red);
    } else {
        // draw smpte bars
        img.fill(Qt::black);

        QPainter p(&img);
        int bar_width = qCeil((double) parent_clip->sequence->width / 7.0);
        int first_bar_height = qCeil((double) parent_clip->sequence->height / 3.0 * 2.0);
        int second_bar_height = qCeil((double) parent_clip->sequence->height / 12.5);
        int third_bar_y = first_bar_height + second_bar_height;
        int third_bar_height = parent_clip->sequence->height - third_bar_y;
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
                third_bar_width = bar_x / SMPTE_LOWER_BARS;

                first_color = QColor(192, 0, 0);
                second_color = QColor(19, 19, 19);

                strip_width = qCeil(bar_width/SMPTE_STRIP_COUNT);
                for (int j=0;j<SMPTE_STRIP_COUNT;j++) {
                    switch (j) {
                    case 0: third_color = QColor(9, 9, 9); break;
                    case 1: third_color = QColor(19, 19, 19); break;
                    case 2: third_color = QColor(29, 29, 29); break;
                    }
                    p.fillRect(QRect(bar_x + (strip_width*j), third_bar_y, strip_width, third_bar_height), third_color);
                }           
                break;
            case 6:
                first_color = QColor(0, 0, 192);
                second_color = QColor(192, 192, 192);
                p.fillRect(QRect(bar_x, third_bar_y, bar_width, third_bar_height), QColor(19, 19, 19));
                break;
            }
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
            p.fillRect(QRect(bar_x, third_bar_y, third_bar_width, third_bar_height), third_color);
        }
    }

    if (texture == NULL) {
        texture = new QOpenGLTexture(img);
    } else {
        texture->destroy();
        texture->setData(img);
    }

    field_changed();
}

void SolidEffect::post_gl() {
    if (texture != NULL) {
        texture->bind();

        int half_width = parent_clip->sequence->width/2;
        int half_height = parent_clip->sequence->height/2;

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
