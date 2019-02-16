#version 110

varying vec2 vTexCoord;
uniform sampler2D texture;
uniform float pixels_x;
uniform float pixels_y;
uniform bool bypass;

void main(void) {
	if (bypass) {
		gl_FragColor = texture2D(texture, vTexCoord);
	} else {
		vec2 p = vTexCoord;

		p.x -= mod(p.x, 1.0 / pixels_x);
		p.y -= mod(p.y, 1.0 / pixels_y);

		gl_FragColor = texture2D(texture, p);
	}  	
}