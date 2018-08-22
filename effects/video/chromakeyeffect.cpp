#include "chromakeyeffect.h"

ChromaKeyEffect::ChromaKeyEffect(Clip* c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_CHROMAKEY_EFFECT) {
	enable_opengl = true;

	color_field = add_row("Color:")->add_field(EFFECT_FIELD_COLOR);
	tolerance_field = add_row("Tolerance:")->add_field(EFFECT_FIELD_DOUBLE);

	color_field->set_color_value(Qt::green);
	tolerance_field->set_double_default_value(50);
}

void ChromaKeyEffect::process_gl(double timecode, QOpenGLShaderProgram &shaders, GLTextureCoords&) {

}
