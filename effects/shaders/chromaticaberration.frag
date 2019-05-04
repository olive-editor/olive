uniform float red_amount;
uniform float green_amount;
uniform float blue_amount;

uniform vec2 resolution;

vec4 process(vec4 col) {
	vec2 rOffset = vec2(red_amount*0.01)/resolution;
	vec2 gOffset = vec2(green_amount*0.01)/resolution;
	vec2 bOffset = vec2(blue_amount*0.01)/resolution;

    vec4 rValue = texture2D(texture, v_texcoord - rOffset);
    vec4 gValue = texture2D(texture, v_texcoord - gOffset);
    vec4 bValue = texture2D(texture, v_texcoord - bOffset);

    return vec4(rValue.r, gValue.g, bValue.b, col.a);
}