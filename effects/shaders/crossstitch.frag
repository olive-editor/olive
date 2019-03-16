#version 110

uniform sampler2D tex0; 
uniform float time;
uniform vec2 resolution;
uniform float stitching_size;
uniform bool invert;

varying vec2 vTexCoord;

vec4 PostFX(sampler2D tex, vec2 uv, float time) {
	vec4 c = vec4(0.0);
	float size = stitching_size;
	vec2 cPos = uv * resolution;
	vec2 tlPos = floor(cPos / vec2(size, size));
	tlPos *= size;
	int remX = int(mod(cPos.x, size));
	int remY = int(mod(cPos.y, size));
	if (remX == 0 && remY == 0)
		tlPos = cPos;
	vec2 blPos = tlPos;
	blPos.y += (size - 1.0);
	if ((remX == remY) || 
		 (((int(cPos.x) - int(blPos.x)) == (int(blPos.y) - int(cPos.y)))))
	{
		if (!invert)
			c = vec4(0.2, 0.15, 0.05, 1.0);
		else
			c = texture2D(tex, tlPos * vec2(1.0/resolution.x, 1.0/resolution.y)) * 1.4;
	}
	else
	{
		if (!invert)
			c = texture2D(tex, tlPos * vec2(1.0/resolution.x, 1.0/resolution.y)) * 1.4;
		else
			c = vec4(0.0, 0.0, 0.0, 1.0);
	}
	return c;
}

void main(void) {
	vec2 uv = vTexCoord;
	gl_FragColor = PostFX(tex0, uv, time);
}