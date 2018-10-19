#version 110

// uniform float evolution;
uniform float frequency;
uniform float intensity;
uniform float evolution;

uniform mediump float amount_val;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	float offset = sin((vTexCoord.x-evolution)*frequency)*intensity;

	float y = vTexCoord.y-offset;
	if (y < 0.0 || y > 1.0) {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	} else {
		vec4 textureColor = texture2D(myTexture, vec2(vTexCoord.x, y));
		gl_FragColor = vec4(
			textureColor.r,
			textureColor.g,
			textureColor.b,
			textureColor.a
		);
	}	
}