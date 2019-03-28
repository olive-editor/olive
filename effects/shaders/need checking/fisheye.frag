#define M_PI 3.1415926535897932384626433832795

uniform float size;

vec4 process(vec4 col) {
	float maxFactor = 2.0 - (size * 0.01);

	vec2 uv;
	vec2 xy = 2.0 * v_texcoord - 1.0;
	float d = length(xy);
	if (d < (2.0-maxFactor)) 	{
		d = length(xy * maxFactor);
		float z = sqrt(1.0 - d * d);
		float r = atan(d, z) / M_PI;
		float phi = atan(xy.y, xy.x);
		
		uv.x = r * cos(phi) + 0.5;
		uv.y = r * sin(phi) + 0.5;
	} else {
		uv = v_texcoord;
	}
	return texture2D(texture, uv);
}