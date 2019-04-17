uniform float lensRadiusX;
uniform float lensRadiusY;
uniform float centerX;
uniform float centerY;
uniform bool circular;
uniform vec2 resolution;
// uniform vec2 lensRadius; // 0.45, 0.38

vec4 process(vec4 c) {
	if (lensRadiusX == 0.0) {
		return vec4(0.0);
	}
	vec2 vignetteCoord = v_texcoord;
	if (circular) {
		float ar = (resolution.x/resolution.y);
		vignetteCoord.x *= ar;
		vignetteCoord.x -= (1.0-(1.0/ar));
	}
	float dist = distance(vignetteCoord, vec2(0.5 + centerX*0.01, 0.5 + centerY*0.01));
	float size = (lensRadiusX*0.01);
	return vec4(c.rgb * smoothstep(size, size*0.99*(1.0-lensRadiusY*0.01), dist), c.a);
}
