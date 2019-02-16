#version 110

uniform bool horiz;
uniform bool vert;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	float x = vTexCoord.x;
	float y = vTexCoord.y;

	if (horiz) x = 1.0 - x;
	if (vert) y = 1.0 - y;

	vec4 textureColor = texture2D(myTexture, vec2(x, y));
	gl_FragColor = vec4(
		textureColor.r,
		textureColor.g,
		textureColor.b,
		textureColor.a
	);
}