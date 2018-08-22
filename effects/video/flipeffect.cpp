#include "flipeffect.h"

FlipEffect::FlipEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_FLIP_EFFECT) {
	enable_opengl = true;

	horizontal_field = add_row("Horizontal:")->add_field(EFFECT_FIELD_BOOL);
	vertical_field = add_row("Vertical:")->add_field(EFFECT_FIELD_BOOL);

	connect(horizontal_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(vertical_field, SIGNAL(changed()), this, SLOT(field_changed()));
}

void FlipEffect::process_gl(double timecode, QOpenGLShaderProgram &, GLTextureCoords &coords) {
	if (horizontal_field->get_bool_value(timecode)) {
		double tlX = coords.textureTopLeftX;
		double blX = coords.textureBottomLeftX;
		double trX = coords.textureTopRightX;
		double brX = coords.textureBottomRightX;
		coords.textureTopLeftX = trX;
		coords.textureTopRightX = tlX;
		coords.textureBottomLeftX = brX;
		coords.textureBottomRightX = blX;
	}
	if (vertical_field->get_bool_value(timecode)) {
		double tlY = coords.textureTopLeftY;
		double blY = coords.textureBottomLeftY;
		double trY = coords.textureTopRightY;
		double brY = coords.textureBottomRightY;
		coords.textureTopLeftY = blY;
		coords.textureTopRightY = brY;
		coords.textureBottomLeftY = tlY;
		coords.textureBottomRightY = trY;
	}
}


