#version 110

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D image;

uniform float radius;
uniform vec2 resolution;
uniform bool horiz_blur;
uniform bool vert_blur;

void main(void) {
	if (radius == 0.0 || (!horiz_blur && !vert_blur)) {
		gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution);
	} else {
		float divider = 1.0 / ((radius*2.0)+1.0);
		if (horiz_blur && vert_blur) {
			divider *= divider;

			gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution)*divider;

			for (float i=1.0;i<=radius;i++) {
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-i, gl_FragCoord.y))/resolution)*(divider);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+i, gl_FragCoord.y))/resolution)*(divider);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y+i))/resolution)*(divider);
				gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y-i))/resolution)*(divider);
			}

			for (float x=1.0;x<=radius;x++) {
				for (float y=1.0;y<=radius;y++) {
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-x, gl_FragCoord.y-y))/resolution)*(divider);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y+y))/resolution)*(divider);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-x, gl_FragCoord.y+y))/resolution)*(divider);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y-y))/resolution)*(divider);
				}
			}
		} else {
			gl_FragColor = texture2D(image, gl_FragCoord.xy/resolution)*(divider);
			
			for (float i=1.0;i<=radius;i++) {
				if (horiz_blur) {
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x-i, gl_FragCoord.y))/resolution)*(divider);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x+i, gl_FragCoord.y))/resolution)*(divider);
				} else {
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y-i))/resolution)*(divider);
					gl_FragColor += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y+i))/resolution)*(divider);
				}
			}
		}		
	}	
}