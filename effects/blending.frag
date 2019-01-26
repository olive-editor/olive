#version 110

const int BLEND_MODE_ADD = 0;
const int BLEND_MODE_AVERAGE = 1;
const int BLEND_MODE_COLORBURN = 2;
const int BLEND_MODE_COLORDODGE = 3;
const int BLEND_MODE_DARKEN = 4;
const int BLEND_MODE_DIFFERENCE = 5;
const int BLEND_MODE_EXCLUSION = 6;
const int BLEND_MODE_GLOW = 7;
const int BLEND_MODE_HARDLIGHT = 8;
const int BLEND_MODE_HARDMIX = 9;
const int BLEND_MODE_LIGHTEN = 10;
const int BLEND_MODE_LINEARBURN = 11;
const int BLEND_MODE_LINEARDODGE = 12;
const int BLEND_MODE_LINEARLIGHT = 13;
const int BLEND_MODE_MULTIPLY = 14;
const int BLEND_MODE_NEGATION = 15;
const int BLEND_MODE_NORMAL = 16;
const int BLEND_MODE_OVERLAY = 17;
const int BLEND_MODE_PHOENIX = 18;
const int BLEND_MODE_PINLIGHT = 19;
const int BLEND_MODE_REFLECT = 20;
const int BLEND_MODE_SCREEN = 21;
const int BLEND_MODE_SOFTLIGHT = 22;
const int BLEND_MODE_SUBSTRACT = 23;
const int BLEND_MODE_SUBTRACT = 24;
const int BLEND_MODE_VIVIDLIGHT = 25;

uniform sampler2D background;
uniform sampler2D texture;
varying vec2 vTexCoord;

void main(void) {
	gl_FragColor = texture2D(background, vTexCoord);
	// gl_FragColor = texture2D(texture, vTexCoord)*2.0;
}