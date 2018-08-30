#version 110

uniform vec4 solidColor;
uniform float amount_val;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vTexCoord);
	gl_FragColor = vec4(
		textureColor.r+((solidColor.r-textureColor.r)*amount_val),
		textureColor.g+((solidColor.g-textureColor.g)*amount_val),
		textureColor.b+((solidColor.b-textureColor.b)*amount_val),
		textureColor.a
	);	
}