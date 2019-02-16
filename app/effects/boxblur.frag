#version 110

uniform sampler2D image;

uniform float radius;
uniform vec2 resolution;
uniform bool horiz_blur;
uniform bool vert_blur;
uniform int iteration;

varying vec2 vTexCoord;

void main(void) {
	float rad = ceil(radius);
	
	float divider = 1.0 / rad;		
	vec4 color = vec4(0.0);
	bool radius_is_zero = (rad == 0.0);

	if (iteration == 0 && horiz_blur && !radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {					
			color += texture2D(image, (vec2(gl_FragCoord.x+x, gl_FragCoord.y))/resolution)*(divider);
		}
		gl_FragColor = color;
	} else if (iteration == 1 && vert_blur && !radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {					
			color += texture2D(image, (vec2(gl_FragCoord.x, gl_FragCoord.y+x))/resolution)*(divider);
		}
		gl_FragColor = color;
	} else {
		gl_FragColor = texture2D(image, vTexCoord);
	}
}