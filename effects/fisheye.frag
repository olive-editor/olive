#version 120
uniform sampler2D tex0;
varying vec2 vTexCoord;
#define M_PI 3.1415926535897932384626433832795

uniform float size;

void main(void) {
	float maxFactor = 2.0 - (size * 0.01);

	vec2 uv;
	vec2 xy = 2.0 * vTexCoord - 1.0;
	float d = length(xy);
	if (d < (2.0-maxFactor)) 	{
		d = length(xy * maxFactor);
		float z = sqrt(1.0 - d * d);
		float r = atan(d, z) / M_PI;
		float phi = atan(xy.y, xy.x);
		
		uv.x = r * cos(phi) + 0.5;
		uv.y = r * sin(phi) + 0.5;
	} else {
		uv = vTexCoord;
	}
	vec4 c = texture2D(tex0, uv);
	gl_FragColor = c;
}