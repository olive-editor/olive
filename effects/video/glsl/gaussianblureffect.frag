#version 110

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D image;

uniform float radius;
uniform float sigma;
uniform vec2 resolution;
uniform bool horiz_blur;
uniform bool vert_blur;

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
		float middleWeight = gaussian(0.0, sigma);
		float sum = middleWeight;

		if (horiz_blur && vert_blur) {
			for (float i=1.0;i<=radius;i++) {
				sum += (4.0*gaussian2(i, 0.0, sigma));
			}

			for (float x=1.0;x<=radius;x++) {
				for (float y=1.0;y<=radius;y++) {
					sum += (4.0*gaussian2(x, y, sigma));
				}
			}
			
			gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution)*(middleWeight/sum);

			for (float i=1.0;i<=radius;i++) {
				float weight = (gaussian2(i, 0.0, sigma)/sum);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-i, gl_FragCoord.y))/resolution)*(weight);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+i, gl_FragCoord.y))/resolution)*(weight);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y+i))/resolution)*(weight);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y-i))/resolution)*(weight);
			}

			for (float x=1.0;x<=radius;x++) {
				for (float y=1.0;y<=radius;y++) {
					float weight = (gaussian2(x, y, sigma)/sum);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-x, gl_FragCoord.y-y))/resolution)*(weight);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y+y))/resolution)*(weight);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-x, gl_FragCoord.y+y))/resolution)*(weight);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y-y))/resolution)*(weight);
				}
			}
		} else {
			for (float i=1.0;i<=radius;i++) {
				sum += (2.0*gaussian(i, sigma));
			}

			gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution)*(middleWeight/sum);
			
			for (float i=1.0;i<=radius;i++) {
				float weight = gaussian(i, sigma)/sum;
				if (horiz_blur) {
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-i, gl_FragCoord.y))/resolution)*(weight);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+i, gl_FragCoord.y))/resolution)*(weight);
				} else {
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y-i))/resolution)*(weight);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y+i))/resolution)*(weight);
				}
			}
		}		
	}	
}