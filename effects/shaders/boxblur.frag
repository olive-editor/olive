uniform float radius;
uniform bool horiz_blur;
uniform bool vert_blur;
uniform int iteration;
uniform vec2 resolution;

vec4 process(vec4 col) {
	float rad = ceil(radius);
	
	float divider = 1.0 / rad;		
	vec4 color = vec4(0.0);
	bool radius_is_zero = (rad == 0.0);

	if (iteration == 0 && horiz_blur && !radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {					
			color += texture2D(texture, vec2(gl_FragCoord.x+x, gl_FragCoord.y)/resolution)*(divider);
		}
		return color;
	} else if (iteration == 1 && vert_blur && !radius_is_zero) {
		for (float x=-rad+0.5;x<=rad;x+=2.0) {					
			color += texture2D(texture, vec2(gl_FragCoord.x, gl_FragCoord.y+x)/resolution)*(divider);
		}
		return color;
	} else {
		return col;
	}
}
