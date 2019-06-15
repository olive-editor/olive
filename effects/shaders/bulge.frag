const float PI = 3.1415926535;

uniform vec2 resolution;
uniform float amount;
uniform float xoff;
uniform float yoff;

vec2 distort(vec2 p, vec2 offset) {
    float theta  = atan(p.y, p.x);
    float radius = length(p);
    radius = pow(radius, (1.0+amount*0.01));
    p.x = radius * cos(theta) + offset.x;
    p.y = radius * sin(theta) + offset.y;
    return 0.5 * (p + 1.0);
}

vec4 process(vec4 col) {
	vec2 offset = vec2(xoff/resolution.x, yoff/resolution.y);
	vec2 xy = 2.0 * v_texcoord - 1.0 - offset;
	vec2 uv;
	float d = length(xy);
	uv = distort(xy, offset);
	if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0) {
		return texture2D(texture, uv);
	} else {
		return vec4(0.0);
	}	
}