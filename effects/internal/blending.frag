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
uniform sampler2D foreground;

uniform int blendmode;
uniform float opacity;

varying vec2 vTexCoord;

// float blending functions
float blend_color_burn(float base, float blend) {
	return (blend==0.0)?blend:max((1.0-((1.0-base)/blend)),0.0);
}

float blend_color_dodge(float base, float blend) {
	return (blend==1.0)?blend:min(base/(1.0-blend),1.0);
}

float blend_vivid_light(float base, float blend) {
	return (blend<0.5)?blend_color_burn(base,(2.0*blend)):blend_color_dodge(base,(2.0*(blend-0.5)));
}

vec3 blend_vivid_light(vec3 base, vec3 blend) {
	return vec3(blend_vivid_light(base.r,blend.r),blend_vivid_light(base.g,blend.g),blend_vivid_light(base.b,blend.b));
}

float blend_hard_mix(float base, float blend) {
	return (blend_vivid_light(base,blend)<0.5)?0.0:1.0;
}

float blend_lighten(float base, float blend) {
	return max(blend,base);
}

float blend_overlay(float base, float blend) {
	return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));
}

vec3 blend_overlay(vec3 base, vec3 blend) {
	return vec3(blend_overlay(base.r,blend.r),blend_overlay(base.g,blend.g),blend_overlay(base.b,blend.b));
}

float blend_darken(float base, float blend) {
	return min(blend, base);
}

float blend_linear_burn(float base, float blend) {
	return max(base+blend-1.0,0.0);
}

vec3 blend_linear_burn(vec3 base, vec3 blend) {
	return max(base+blend-vec3(1.0),vec3(0.0));
}

float blend_linear_dodge(float base, float blend) {
	return min(base+blend,1.0);
}

vec3 blend_linear_dodge(vec3 base, vec3 blend) {
	return min(base+blend,vec3(1.0));
}

float blend_linear_light(float base, float blend) {
	return blend<0.5?blend_linear_burn(base,(2.0*blend)):blend_linear_dodge(base,(2.0*(blend-0.5)));
}

float blend_pin_light(float base, float blend) {
	return (blend<0.5)?blend_darken(base,(2.0*blend)):blend_lighten(base,(2.0*(blend-0.5)));
}

float blend_reflect(float base, float blend) {
	return (blend==1.0)?blend:min(base*base/(1.0-blend),1.0);
}

vec3 blend_reflect(vec3 base, vec3 blend) {
	return vec3(blend_reflect(base.r,blend.r),blend_reflect(base.g,blend.g),blend_reflect(base.b,blend.b));
}

float blend_screen(float base, float blend) {
	return 1.0-((1.0-base)*(1.0-blend));
}

float blend_substract(float base, float blend) {
	return max(base+blend-1.0,0.0);
}

float blend_soft_light(float base, float blend) {
	return (blend<0.5)?(2.0*base*blend+base*base*(1.0-2.0*blend)):(sqrt(base)*(2.0*blend-1.0)+2.0*base*(1.0-blend));
}

// adapted from https://github.com/jamieowen/glsl-blend
// RGB blending function, alpha is handled below
vec3 blend(vec3 base, vec3 blend) {
	switch (blendmode) {

	case BLEND_MODE_AVERAGE:
		return (base+blend)/2.0;

	case BLEND_MODE_COLORBURN:
		return vec3(blend_color_burn(base.r, blend.r), blend_color_burn(base.g, blend.g), blend_color_burn(base.b, blend.b));

	case BLEND_MODE_COLORDODGE:
		return vec3(blend_color_dodge(base.r, blend.r), blend_color_dodge(base.g, blend.g), blend_color_dodge(base.b, blend.b));

	case BLEND_MODE_DARKEN:
		return vec3(blend_darken(base.r, blend.r), blend_darken(base.g, blend.g), blend_darken(base.b, blend.b));

	case BLEND_MODE_DIFFERENCE:
		return abs(base-blend);

	case BLEND_MODE_EXCLUSION:
		return base+blend-2.0*base*blend;

	case BLEND_MODE_GLOW:
		return blend_reflect(blend, base);

	case BLEND_MODE_HARDLIGHT:
		return blend_overlay(blend,base);

	case BLEND_MODE_HARDMIX:
		return vec3(blend_hard_mix(base.r,blend.r),blend_hard_mix(base.g,blend.g),blend_hard_mix(base.b,blend.b));

	case BLEND_MODE_LIGHTEN:
		return vec3(blend_lighten(base.r,blend.r),blend_lighten(base.g,blend.g),blend_lighten(base.b,blend.b));

	case BLEND_MODE_LINEARBURN:
	case BLEND_MODE_SUBTRACT:
		return blend_linear_burn(base, blend);

	case BLEND_MODE_ADD:
	case BLEND_MODE_LINEARDODGE:
		return blend_linear_dodge(base, blend);

	case BLEND_MODE_LINEARLIGHT:
		return vec3(blend_linear_light(base.r,blend.r),blend_linear_light(base.g,blend.g),blend_linear_light(base.b,blend.b));

	case BLEND_MODE_MULTIPLY:
		return (base * blend);

	case BLEND_MODE_NEGATION:
		return vec3(1.0)-abs(vec3(1.0)-base-blend);

	case BLEND_MODE_OVERLAY:
		return blend_overlay(base, blend);

	case BLEND_MODE_PHOENIX:
		return min(base,blend)-max(base,blend)+vec3(1.0);

	case BLEND_MODE_PINLIGHT:
		return vec3(blend_pin_light(base.r,blend.r),blend_pin_light(base.g,blend.g),blend_pin_light(base.b,blend.b));

	case BLEND_MODE_REFLECT:
		return blend_reflect(base, blend);

	case BLEND_MODE_SCREEN:
		return vec3(blend_screen(base.r,blend.r),blend_screen(base.g,blend.g),blend_screen(base.b,blend.b));

	case BLEND_MODE_SUBSTRACT:
		return max(base+blend-vec3(1.0),vec3(0.0));

	case BLEND_MODE_SOFTLIGHT:
		return vec3(blend_soft_light(base.r,blend.r),blend_soft_light(base.g,blend.g),blend_soft_light(base.b,blend.b));

	case BLEND_MODE_VIVIDLIGHT:
		return vec3(blend_vivid_light(base.r,blend.r),blend_vivid_light(base.g,blend.g),blend_vivid_light(base.b,blend.b));

	case BLEND_MODE_NORMAL:
	default:
		return blend;

	}
}

void main(void) {
	vec4 bg_color = texture2D(background, vTexCoord);
	vec4 fg_color = texture2D(foreground, vTexCoord);

	// blend textures together
	vec3 composite = blend(bg_color.rgb, fg_color.rgb);	

	// add foreground and background alpha's together
	vec4 full_composite = vec4(composite, bg_color.a + fg_color.a);

	// restore background texture based on foreground's alpha
	bool restore_bg = true;

	// some blend modes don't need this
	switch (blendmode) {
	case BLEND_MODE_SCREEN:
	case BLEND_MODE_LIGHTEN:
	case BLEND_MODE_COLORDODGE:
	case BLEND_MODE_LINEARDODGE:
	case BLEND_MODE_ADD:
	// case BLEND_MODE_OVERLAY:
	case BLEND_MODE_SOFTLIGHT:
	case BLEND_MODE_DIFFERENCE:
	case BLEND_MODE_AVERAGE:
	case BLEND_MODE_NEGATION:
	case BLEND_MODE_PHOENIX:
		restore_bg = false;
	}

	if (restore_bg) {
		full_composite += vec4(bg_color.rgb*(1.0-fg_color.a), 0.0);
	}

	// mix via opacity
	full_composite = mix(bg_color, full_composite, opacity);

	// output to color
	gl_FragColor = full_composite;
}