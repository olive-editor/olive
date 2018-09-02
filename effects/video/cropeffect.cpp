#include "cropeffect.h"

CropEffect::CropEffect(Clip *c) : Effect(c, EFFECT_TYPE_VIDEO, VIDEO_CROP_EFFECT) {
	enable_coords = true;

	left_field = add_row("Left:")->add_field(EFFECT_FIELD_DOUBLE);
	top_field = add_row("Top:")->add_field(EFFECT_FIELD_DOUBLE);
	right_field = add_row("Right:")->add_field(EFFECT_FIELD_DOUBLE);
	bottom_field = add_row("Bottom:")->add_field(EFFECT_FIELD_DOUBLE);

	left_field->set_double_minimum_value(0);
	left_field->set_double_maximum_value(100);
	top_field->set_double_minimum_value(0);
	top_field->set_double_maximum_value(100);
	right_field->set_double_minimum_value(0);
	right_field->set_double_maximum_value(100);
	bottom_field->set_double_minimum_value(0);
	bottom_field->set_double_maximum_value(100);

	connect(left_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(top_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(right_field, SIGNAL(changed()), this, SLOT(field_changed()));
	connect(bottom_field, SIGNAL(changed()), this, SLOT(field_changed()));
}

void CropEffect::process_coords(double timecode, GLTextureCoords &coords) {
	// store initial coord data
	int left = coords.vertexTopLeftX;
	int top = coords.vertexTopLeftY;
	int right = coords.vertexBottomRightX;
	int bottom = coords.vertexBottomRightY;
	int width = right - left;
	int height = bottom - top;

	double texLeft = coords.textureTopLeftX;
	double texTop = coords.textureTopLeftY;
	double texRight = coords.textureBottomRightX;
	double texBottom = coords.textureBottomRightY;
	double texWidth = texRight - texLeft;
	double texHeight = texBottom - texTop;

	// retrieve values
	double left_field_value = (left_field->get_double_value(timecode)*0.01);
	double right_field_value = 1-(right_field->get_double_value(timecode)*0.01);
	double top_field_value = (top_field->get_double_value(timecode)*0.01);
	double bottom_field_value = 1-(bottom_field->get_double_value(timecode)*0.01);

	// validate values
	if (left_field_value > right_field_value) right_field_value = left_field_value;
	if (top_field_value > bottom_field_value) bottom_field_value = top_field_value;

	// left
	coords.textureTopLeftX = coords.textureBottomLeftX = texLeft+(left_field_value*texWidth);
	coords.vertexTopLeftX = coords.vertexBottomLeftX = left+(width*left_field_value);

	// right
	coords.textureTopRightX = coords.textureBottomRightX = texLeft+(right_field_value*texWidth);
	coords.vertexTopRightX = coords.vertexBottomRightX = left+(width*right_field_value);

	// top
	coords.textureTopLeftY = coords.textureTopRightY = texTop+(top_field_value*texHeight);
	coords.vertexTopLeftY = coords.vertexTopRightY = top+(height*top_field_value);

	// bottom
	coords.textureBottomLeftY = coords.textureBottomRightY = texTop+(bottom_field_value*texHeight);
	coords.vertexBottomLeftY = coords.vertexBottomRightY = top+(height*bottom_field_value);
}
