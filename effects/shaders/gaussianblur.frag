#version 110

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D image;

// uniform float radius;
uniform float sigma;
uniform vec2 resolution;
uniform bool horiz_blur;
uniform bool vert_blur;
uniform int iteration;

varying vec2 vTexCoord;

float gaussian(float x, float sigma) {
	return (1.0/(sigma*sqrt(2.0*M_PI)))*exp(-0.5*pow(x/sigma, 2.0));
}

float gaussian2(float x, float y, float sigma) {
	return (1.0/(pow(sigma, 2.0)*2.0*M_PI))*exp(-0.5*((pow(x, 2.0) + pow(y, 2.0))/pow(sigma, 2.0)));
}

void main(void) {
	float rad = ceil(sigma);

	float sum = 0.0;

	vec4 color = vec4(0.0);

	bool radius_is_zero = (rad == 0.0);

	if (!radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {
			sum += gaussian2(x, 0.0, sigma);
		}
	}

	if (iteration == 0 && horiz_blur && !radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {
			float weight = (gaussian2(x, 0.0, sigma)/sum);
			color += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y))/resolution)*(weight);
		}
		gl_FragColor = color;
	} else if (iteration == 1 && vert_blur && !radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {
			float weight = (gaussian2(0.0, x, sigma)/sum);
			color += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y+x))/resolution)*(weight);
		}
		gl_FragColor = color;
	} else {
		gl_FragColor = texture2D(image, vTexCoord);
	}
}