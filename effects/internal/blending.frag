#version 110

/*
	This main stump is combined with another shader loaded externally to produce a blending mode.

	For a custom blend mode, the function blend() MUST be available, and use the following syntax:

	blend(vec3 base_color, vec3 blend_color, float opacity)

*/

void main(void) {
	vec4 bg_color = texture2D(background, vTexCoord);
	vec4 fg_color = texture2D(foreground, vTexCoord);

	// blend textures together
	vec3 composite = blend(bg_color.rgb, fg_color.rgb);	

	if (blendmode == BLEND_MODE_OVERLAY
			|| blendmode == BLEND_MODE_LIGHTEN 
			|| blendmode == BLEND_MODE_SCREEN 
			|| blendmode == BLEND_MODE_COLORDODGE 
			|| blendmode == BLEND_MODE_LINEARDODGE 
			|| blendmode == BLEND_MODE_ADD 
			|| blendmode == BLEND_MODE_SOFTLIGHT 
			|| blendmode == BLEND_MODE_NEGATION 
			|| blendmode == BLEND_MODE_AVERAGE 
			|| blendmode == BLEND_MODE_REFLECT 
			|| blendmode == BLEND_MODE_EXCLUSION 
			|| blendmode == BLEND_MODE_DIFFERENCE) {
		composite *= fg_color.a;
	}

	vec4 full_composite = vec4(composite + bg_color.rgb*(1.0-fg_color.a), bg_color.a + fg_color.a);

	full_composite = mix(bg_color, full_composite, opacity);

	// output to color
	gl_FragColor = full_composite;
}