uniform float speed;
uniform float intensity;
uniform float frequency;
uniform float xoff;
uniform float yoff;
uniform bool reverse;
uniform bool stretch;

uniform vec2 resolution; // Screen resolution
uniform float time; // time in seconds

vec4 process(vec4 col) {
	vec2 texCoord = v_texcoord;
	vec2 center = vec2(1.0);

	if (!stretch) {
		texCoord.x *= (resolution.x/resolution.y);
		center.x += (resolution.x/resolution.y)/2.0;
	}

	center += vec2(xoff, yoff)*0.01;

	float real_time = (reverse) ? -time : time;

	vec2 p = 2.0 * texCoord - center;
	float len = length(p);
	vec2 uv = v_texcoord + (p/len)*cos((frequency*0.01)*(len*12.0-real_time*(speed*0.05)))*(intensity*0.0005);
	return texture2D(texture, uv);  
}