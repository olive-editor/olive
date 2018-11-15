#version 110

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D image;

uniform float angle; // degrees
uniform float length;

uniform vec2 resolution;

void main(void) {
	float radians = (angle*M_PI)/180.0;
	float divider = 1.0 / ((length*2.0)+1.0);
	float sin_angle = sin(radians);
	float cos_angle = cos(radians);

	gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution)*(divider);
	for (float i=1.0;i<=length;i++) {
		float y = sin_angle * i;
		float x = cos_angle * i;

		gl_FragColor += texture2D(image, vec2(gl_FragCoord.x-x, gl_FragCoord.y-y)/resolution)*(divider);
		gl_FragColor += texture2D(image, vec2(gl_FragCoord.x+x, gl_FragCoord.y+y)/resolution)*(divider);
	}
}