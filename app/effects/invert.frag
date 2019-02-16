#version 110

uniform float amount;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vTexCoord);
	float amount_val = amount * 0.01;
	vec3 col = textureColor.rgb+((vec3(1.0)-textureColor.rgb-textureColor.rgb)*vec3(amount_val));
	gl_FragColor = vec4(col, textureColor.a);
}