#version 110

uniform float frequency;
uniform float intensity;
uniform float evolution;
uniform bool vertical;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	float x = vTexCoord.x;
	float y = vTexCoord.y;

	if (vertical) {
		x -= sin((vTexCoord.y-(evolution*0.01))*frequency)*intensity*0.01;
	} else {
		y -= sin((vTexCoord.x-(evolution*0.01))*frequency)*intensity*0.01;
	}

	if (y < 0.0 || y > 1.0 || x < 0.0 || x > 1.0) {
		discard;
	} else {
		vec4 textureColor = texture2D(myTexture, vec2(x, y));
		gl_FragColor = vec4(
			textureColor.r,
			textureColor.g,
			textureColor.b,
			textureColor.a
		);
	}	
}