uniform float gamma_cent; // 0.6
uniform float numColors; // 8.0

vec4 process(vec4 color) {
	float gamma = gamma_cent*0.01;
	vec3 c = color.rgb;
	c = pow(c, vec3(gamma, gamma, gamma));
	c = c * numColors;
	c = floor(c);
	c = c / numColors;
	c = pow(c, vec3(1.0/gamma));
	return vec4(c, color.a);
}