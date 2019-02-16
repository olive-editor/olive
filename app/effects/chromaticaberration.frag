#version 110

uniform vec2 resolution;

uniform sampler2D tex;
varying vec2 vTexCoord;

uniform float red_amount;
uniform float green_amount;
uniform float blue_amount;

void main(void) {
	vec2 rOffset = vec2(red_amount*0.01)/resolution;
	vec2 gOffset = vec2(green_amount*0.01)/resolution;
	vec2 bOffset = vec2(blue_amount*0.01)/resolution;

    vec4 rValue = texture2D(tex, vTexCoord - rOffset);
    vec4 gValue = texture2D(tex, vTexCoord - gOffset);
    vec4 bValue = texture2D(tex, vTexCoord - bOffset);

    gl_FragColor = vec4(rValue.r, gValue.g, bValue.b, texture2D(tex, vTexCoord).a);
}