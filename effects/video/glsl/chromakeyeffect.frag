#version 110

uniform vec4 keyColor;
uniform float threshold;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vTexCoord);
	float diff = length(keyColor - textureColor);
	if (diff < threshold) {
		gl_FragColor = vec4(0, 0, 0, 0);
	} else {
		gl_FragColor = textureColor;
	}
}