// Swirl effect parameters
uniform float radius;
uniform float angle;
uniform float center_x;
uniform float center_y;
uniform vec2 resolution;

vec4 process(vec4 col) {
	vec2 center = vec2((resolution.x*0.5)+center_x, (resolution.y*0.5)+center_y);

	vec2 uv = v_texcoord.st;

	vec2 tc = uv * resolution;
	tc -= center;
	float dist = length(tc);
	if (dist < radius) {
		float percent = (radius - dist) / radius;
		float theta = percent * percent * (-angle*0.05) * 8.0;
		float s = sin(theta);
		float c = cos(theta);
		tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));
	}
	tc += center;

	return texture2D(texture, tc / resolution);
}