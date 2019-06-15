uniform float amount;

vec4 process(vec4 col) {
	float amount_val = amount * 0.01;
	vec3 color = col.rgb+((vec3(1.0)-col.rgb-col.rgb)*vec3(amount_val));
	return vec4(color, col.a);
}