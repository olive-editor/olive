#include "solideffect.h"

#include <QOpenGLTexture>
#include <QPainter>
#include <QtMath>
#include <QVariant>

#include "project/clip.h"
#include "project/sequence.h"

#define SOLID_TYPE_COLOR 0
#define SOLID_TYPE_BARS 1

#define SMPTE_BARS 7
#define SMPTE_STRIP_COUNT 3
#define SMPTE_LOWER_BARS 4

SolidEffect::SolidEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_SOLID_EFFECT), vert(QOpenGLShader::Vertex), frag(QOpenGLShader::Fragment) {
	enable_opengl = true;

	solid_type = add_row("Type:")->add_field(EFFECT_FIELD_COMBO);
	solid_type->add_combo_item("Solid Color", SOLID_TYPE_COLOR);
	solid_type->add_combo_item("SMPTE Bars", SOLID_TYPE_BARS);

	solid_color_field = add_row("Color:")->add_field(EFFECT_FIELD_COLOR);
	solid_color_field->set_color_value(Qt::red);

	opacity_field = add_row("Opacity:")->add_field(EFFECT_FIELD_DOUBLE);
	opacity_field->set_double_minimum_value(0);
	opacity_field->set_double_maximum_value(100);
	opacity_field->set_double_default_value(100);

    connect(solid_type, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(solid_color_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(opacity_field, SIGNAL(changed()), this, SLOT(field_changed()));

    vert.compileSourceCode("varying vec2 vTexCoord;\nvoid main() {\n\tvTexCoord = gl_MultiTexCoord0.xy;\n\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n}");
    frag.compileSourceCode("uniform vec4 solidColor;\nuniform float amount_val;\nuniform sampler2D myTexture;\nvarying vec2 vTexCoord;\nvoid main(void) {\n\tvec4 textureColor = texture2D(myTexture, vTexCoord);\n\tgl_FragColor = vec4(textureColor.r+((solidColor.r-textureColor.r)*amount_val), textureColor.g+((solidColor.g-textureColor.g)*amount_val), textureColor.b+((solidColor.b-textureColor.b)*amount_val), textureColor.a);\n}");
    program.addShader(&vert);
    program.addShader(&frag);
}

void SolidEffect::process_gl(double timecode, GLTextureCoords&) {
    solid_color_field->set_enabled(solid_type->get_combo_data(timecode) == SOLID_TYPE_COLOR);

    program.bind();
    program.setUniformValue("solidColor", solid_color_field->get_color_value(timecode));
    program.setUniformValue("amount_val", (GLfloat) (opacity_field->get_double_value(timecode)*0.01));
}

void SolidEffect::clean_gl() {
    program.release();
}

void SolidEffect::process_image(long frame, uint8_t* data, int w, int h) {
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
}
