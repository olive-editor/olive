uniform float pixels_x;
uniform float pixels_y;
uniform bool bypass;

vec4 process(vec4 col) {
	if (bypass) {
		return col;
	} else {
		vec2 p = v_texcoord;

		p.x -= mod(p.x, 1.0 / pixels_x);
		p.y -= mod(p.y, 1.0 / pixels_y);

		return texture2D(texture, p);
	}  	
}