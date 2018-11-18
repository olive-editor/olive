#include "solideffect.h"

#include <QOpenGLTexture>
#include <QPainter>
#include <QtMath>
#include <QVariant>
#include <QImage>

#include "project/clip.h"
#include "project/sequence.h"

#define SOLID_TYPE_COLOR 0
#define SOLID_TYPE_BARS 1
#define SOLID_TYPE_CHECKERBOARD 2

#define SMPTE_BARS 7
#define SMPTE_STRIP_COUNT 3
#define SMPTE_LOWER_BARS 4

#define CHECKER_SQUARE_COUNT 7

SolidEffect::SolidEffect(Clip* c, const EffectMeta* em) : Effect(c, em) {
	enable_superimpose = true;

	solid_type = add_row("Type:")->add_field(EFFECT_FIELD_COMBO);
	solid_type->add_combo_item("Solid Color", SOLID_TYPE_COLOR);
	solid_type->add_combo_item("SMPTE Bars", SOLID_TYPE_BARS);
    solid_type->add_combo_item("Checkerboard", SOLID_TYPE_CHECKERBOARD);
	solid_type->id = "type";

	solid_color_field = add_row("Color:")->add_field(EFFECT_FIELD_COLOR);
	solid_color_field->set_color_value(Qt::red);
	solid_color_field->id = "color";

	opacity_field = add_row("Opacity:")->add_field(EFFECT_FIELD_DOUBLE);
	opacity_field->set_double_minimum_value(0);
	opacity_field->set_double_maximum_value(100);
	opacity_field->set_double_default_value(100);
	opacity_field->id = "opacity";

    connect(solid_type, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(solid_color_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(opacity_field, SIGNAL(changed()), this, SLOT(field_changed()));

	vertPath = ":/shaders/common.vert";
	fragPath = ":/shaders/solideffect.frag";
}

/*void SolidEffect::process_gl(double timecode, GLTextureCoords&) {


	glslProgram->setUniformValue("solidColor", solid_color_field->get_color_value(timecode));
	glslProgram->setUniformValue("amount_val", (GLfloat) (opacity_field->get_double_value(timecode)*0.01));
}*/

void SolidEffect::redraw(double timecode) {
	int w = img.width();
	int h = img.height();
	int alpha = (opacity_field->get_double_value(timecode)*2.55);
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
        p.setRenderHint(QPainter::Antialiasing);
        img.fill(Qt::transparent);

        int checker_width = qCeil(double(w) / CHECKER_SQUARE_COUNT);
        int checker_height = qCeil(double(h) / CHECKER_SQUARE_COUNT);
        int checker_x, checker_y, count = 0;
        QColor checker_odd(QColor(0,0,0,alpha));
        QColor checker_even(solid_color_field->get_color_value(timecode));
        checker_even.setAlpha(alpha);
        QVector<QColor> checker_color{checker_odd, checker_even};

        for(int i = 0; i < CHECKER_SQUARE_COUNT; i++){
            checker_x = checker_width*i;
            for(int j = 0; j < CHECKER_SQUARE_COUNT; j++){
                  checker_y = checker_height*j;
                  p.fillRect(QRect(checker_x, checker_y, checker_width, checker_height), checker_color[count%2]);
                  count++;
            }
        }
    }
        break;
	}
}

/*void SolidEffect::process_image(long frame, uint8_t* data, int w, int h) {
	QImage img(data, w, h, QImage::Format_RGBA8888); // create QImage wrapper
	QPainter p(&img);
	int width = img.width();
	int height = img.height();

	switch (solid_type->get_combo_data(frame).toInt()) {
	case SOLID_TYPE_COLOR:
	{
		QColor brush = solid_color_field->get_color_value(frame);
		p.fillRect(0, 0, width, height, QColor(brush.red(), brush.green(), brush.blue(), opacity_field->get_double_value(frame)*2.55));
	}
		break;
	case SOLID_TYPE_BARS:
		// draw smpte bars
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
					case 0:
						third_color = QColor(29, 29, 29);
						p.fillRect(QRect(bar_x, third_bar_y, bar_width, third_bar_height), third_color);
						break;
					case 1:
						third_color = QColor(19, 19, 19);
						p.fillRect(QRect(bar_x + strip_width, third_bar_y, strip_width, third_bar_height), third_color);
						break;
					case 2:
						third_color = QColor(9, 9, 9);
						p.fillRect(QRect(bar_x, third_bar_y, strip_width, third_bar_height), third_color);
						break;
					}
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
		break;
	}
}*/
