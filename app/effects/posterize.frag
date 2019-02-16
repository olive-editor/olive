#version 110

uniform sampler2D sceneTex; // 0
uniform float gamma_cent; // 0.6
uniform float numColors; // 8.0

varying vec2 vTexCoord;

void main() {
	float gamma = gamma_cent*0.01;
	vec4 color = texture2D(sceneTex, vTexCoord);
	vec3 c = color.rgb;
	c = pow(c, vec3(gamma, gamma, gamma));
	c = c * numColors;
	c = floor(c);
	c = c / numColors;
	c = pow(c, vec3(1.0/gamma));
	gl_FragColor = vec4(c, color.a);
}