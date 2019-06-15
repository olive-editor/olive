#version 110

uniform sampler2D tex;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(tex, vTexCoord);
	gl_FragColor = textureColor;
}