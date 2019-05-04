#define M_PI 3.1415926535897932384626433832795

uniform float radius;
uniform float center_x;
uniform float center_y;

uniform vec2 resolution;

vec4 process(vec4 col) {
	if (radius > 0.0) {
		vec2 distance = vec2((gl_FragCoord.x - (resolution.x/2.0) - center_x), (gl_FragCoord.y - (resolution.y/2.0) - center_y));

		float angle = atan(distance.y/distance.x);
		float sin_angle = sin(angle);
		float cos_angle = cos(angle);

		float multiplier = length(distance/resolution);

		float limit = ceil(radius * multiplier);

		float divider = 1.0 / limit;

		vec4 color = vec4(0.0);

		for (float i=-limit+0.5;i<=limit;i+=2.0) {
			float y = sin_angle * i;
			float x = cos_angle * i;
			color += texture2D(texture, vec2(gl_FragCoord.x+x, gl_FragCoord.y+y)/resolution)*(divider);
		}
		return color;
	} else {
		return col;
	}
}