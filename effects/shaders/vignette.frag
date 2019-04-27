#version 110

uniform sampler2D sceneTex; // 0
uniform float lensRadiusX;
uniform float lensRadiusY;
uniform float centerX;
uniform float centerY;
uniform bool circular;
uniform vec2 resolution;
uniform bool invert;
// uniform vec2 lensRadius; // 0.45, 0.38

varying vec2 vTexCoord;

void main(void) {
	if (lensRadiusX == 0.0) {
		discard;
	}
	vec4 c = texture2D(sceneTex, vTexCoord);
	vec2 vignetteCoord = vTexCoord;
	if (circular) {
		float ar = (resolution.x/resolution.y);
		vignetteCoord.x *= ar;
		vignetteCoord.x -= (1.0-(1.0/ar));
	}
	float dist = distance(vignetteCoord, vec2(0.5 + centerX*0.01, 0.5 + centerY*0.01));
	float size = (lensRadiusX*0.01);
	
	if (invert)
	{c *= 1.0 - smoothstep(size, size*0.99*(1.0-lensRadiusY*0.01), dist);}
	else
	{c *= smoothstep(size, size*0.99*(1.0-lensRadiusY*0.01), dist);}
	
	gl_FragColor = c;
}