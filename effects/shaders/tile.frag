#version 110

varying vec2 vTexCoord;
uniform sampler2D tex0; // scene buffer

uniform float scale;
uniform float centerx;
uniform float centery;
uniform bool mirrorx;
uniform bool mirrory;

void main(void) {
	vec2 texCoord = vTexCoord;
	
	/*vec2 coord = vec2(
		mod(vTexCoord.x*tilecount, 1.0),
		mod(vTexCoord.y*tilecount, 1.0)
	);
	
	if (mirrorx && mod(vTexCoord.x*tilecount, 2.0) > 1.0) {
		coord.x = 1.0 - coord.x;
	}
	
	if (mirrory && mod(vTexCoord.y*tilecount, 2.0) > 1.0) {
		coord.y = 1.0 - coord.y;
	}*/
	
	float adj_scale = scale*0.01;
	
	vec2 scaled_coords = (vTexCoord/adj_scale);
	vec2 coord = scaled_coords-vec2(0.5/adj_scale, 0.5/adj_scale)+vec2(0.5, 0.5)+vec2(-centerx*0.01, -centery*0.01);
	vec2 modcoord = mod(coord, 1.0);
	
	if (mirrorx && mod(coord.x, 2.0) > 1.0) {
		modcoord.x = 1.0 - modcoord.x;
	}
	
	if (mirrory && mod(coord.y, 2.0) > 1.0) {
		modcoord.y = 1.0 - modcoord.y;
	}

	gl_FragColor = vec4(texture2D(tex0, modcoord));	
}