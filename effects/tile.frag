#version 110

varying vec2 vTexCoord;
uniform sampler2D tex0; // scene buffer

uniform float tilecount;
uniform bool mirrorx;
uniform bool mirrory;

void main(void) {
	vec2 texCoord = vTexCoord;
	
	vec2 coord = vec2(
		mod(vTexCoord.x*tilecount, 1.0),
		mod(vTexCoord.y*tilecount, 1.0)
	);
	
	if (mirrorx && mod(vTexCoord.x*tilecount, 2.0) > 1.0) {
		coord.x = 1.0 - coord.x;
	}
	
	if (mirrory && mod(vTexCoord.y*tilecount, 2.0) > 1.0) {
		coord.y = 1.0 - coord.y;
	}

	gl_FragColor = vec4(texture2D(tex0, coord));	
}