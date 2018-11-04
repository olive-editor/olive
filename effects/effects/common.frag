#version 110

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vec2(vTexCoord.x, vTexCoord.y));
	gl_FragColor = vec4(
		textureColor.r,
		textureColor.g,
		textureColor.b,
		textureColor.a
	);
}