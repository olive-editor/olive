#version 110

uniform sampler2D tex;
varying vec2 vTexCoord;

void main(void) {
	vec4 c = texture2D(tex, vTexCoord);
	c.rgb *= c.a;
	gl_FragColor = c;
}