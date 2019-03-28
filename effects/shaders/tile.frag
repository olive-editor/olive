uniform float scale;
uniform float centerx;
uniform float centery;
uniform bool mirrorx;
uniform bool mirrory;

vec4 process(vec4 col) {

	float adj_scale = scale*0.01;
	
	vec2 scaled_coords = (v_texcoord/adj_scale);
	vec2 coord = scaled_coords-vec2(0.5/adj_scale, 0.5/adj_scale)+vec2(0.5, 0.5)+vec2(-centerx*0.01, -centery*0.01);
	vec2 modcoord = mod(coord, 1.0);
	
	if (mirrorx && mod(coord.x, 2.0) > 1.0) {
		modcoord.x = 1.0 - modcoord.x;
	}
	
	if (mirrory && mod(coord.y, 2.0) > 1.0) {
		modcoord.y = 1.0 - modcoord.y;
	}

	return vec4(texture2D(texture, modcoord));	

}