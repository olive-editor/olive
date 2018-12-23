#version 110

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D image;

uniform float radius;
uniform float sigma;
uniform vec2 resolution;
uniform bool horiz_blur;
uniform bool vert_blur;

uniform bool opt;

float gaussian(float x, float sigma) {
	return (1.0/(sigma*sqrt(2.0*M_PI)))*exp(-0.5*pow(x/sigma, 2.0));
}

float gaussian2(float x, float y, float sigma) {
	return (1.0/(pow(sigma, 2.0)*2.0*M_PI))*exp(-0.5*((pow(x, 2.0) + pow(y, 2.0))/pow(sigma, 2.0)));
}

void main(void) {
	if (radius == 0.0 || sigma == 0.0 || (!horiz_blur && !vert_blur)) {
		gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution);
	} else {
		float rad = ceil(radius);
		float x_rad = horiz_blur ? rad : 0.5;
		float y_rad = vert_blur ? rad : 0.5;

		float sum = 0.0;

		for (float x=-x_rad+0.5;x<=x_rad;x+=2.0) {
			for (float y=-y_rad+0.5;y<=y_rad;y+=2.0) {
				sum += gaussian2(x, y, sigma);
			}
		}

		vec4 color = vec4(0.0);
		for (float x=-x_rad+0.5;x<=x_rad;x+=2.0) {
			for (float y=-y_rad+0.5;y<=y_rad;y+=2.0) {
				float weight = (gaussian2(x, y, sigma)/sum);
				color += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y+y))/resolution)*(weight);
			}
		}
		gl_FragColor = color;
	}	
}