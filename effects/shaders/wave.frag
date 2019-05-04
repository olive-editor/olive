uniform float frequency;
uniform float intensity;
uniform float evolution;
uniform bool vertical;

vec4 process(vec4 col) {
	float x = v_texcoord.x;
	float y = v_texcoord.y;

	if (vertical) {
		x -= sin((v_texcoord.y-(evolution*0.01))*frequency)*intensity*0.01;
	} else {
		y -= sin((v_texcoord.x-(evolution*0.01))*frequency)*intensity*0.01;
	}

	if (y < 0.0 || y > 1.0 || x < 0.0 || x > 1.0) {
		return vec4(0.0);
	} else {
		return texture2D(texture, vec2(x, y));
	}	
}