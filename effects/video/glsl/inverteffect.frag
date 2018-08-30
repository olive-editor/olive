#version 110

uniform mediump float amount_val;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vTexCoord);
	gl_FragColor = vec4(
		textureColor.r+((1.0-textureColor.r-textureColor.r)*amount_val),
		textureColor.g+((1.0-textureColor.g-textureColor.g)*amount_val),
		textureColor.b+((1.0-textureColor.b-textureColor.b)*amount_val),
		textureColor.a
	);
}