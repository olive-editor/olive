#version 110

varying vec2 vTexCoord;

uniform vec2 resolution; // Screen resolution
uniform sampler2D tex0; // scene buffer

uniform float xoff;
uniform float yoff;
uniform float scale;
uniform bool tile;
uniform bool hide_edges;
uniform bool stretch;

void main(void) {
	vec2 texCoord = vTexCoord;

	vec2 offset = vec2(1.0);

	if (!stretch) {
		if (resolution.x > resolution.y) {
			offset.x = (resolution.x/resolution.y);
			texCoord.x *= offset.x;
		} else {
			offset.y = (resolution.y/resolution.x);
			texCoord.y *= offset.y;
		}
	}

	vec2 p = 2.0 * texCoord - offset;
	vec2 adj_tc = 2.0 * vTexCoord - 1.0;
	float r = dot(p,p);
	if (r > 1.0) discard; 
	float f = (1.0-sqrt(1.0-r))/(r);
	vec2 uv;
	uv.x = (adj_tc.x*(1.5-scale*0.01))*f+0.5-(xoff*0.01);
	uv.y = (adj_tc.y*(1.5-scale*0.01))*f+0.5-(yoff*0.01);
	if (tile) {
		uv.x = mod(uv.x, 1.0);
		uv.y = mod(uv.y, 1.0);
	} else if (hide_edges && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)) {
		discard;
	}
	gl_FragColor = vec4(texture2D(tex0,uv));
}