#version 110

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D image;

uniform float angle; // degrees
uniform float length;

uniform vec2 resolution;

void main(void) {
	if (length > 0.0) {
		float ceillen = ceil(length);
		float radians = (angle*M_PI)/180.0;
		float divider = 1.0 / ceillen;
		float sin_angle = sin(radians);
		float cos_angle = cos(radians);

		vec4 color = vec4(0.0);
		for (float i=-ceillen+0.5;i<=ceillen;i+=2.0) {
			float y = sin_angle * i;
			float x = cos_angle * i;
			color += texture2D(image, vec2(gl_FragCoord.x+x, gl_FragCoord.y+y)/resolution)*(divider);
		}
		gl_FragColor = color;
	} else {
		gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution);
	}
}