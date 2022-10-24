#define LINEAR_CURVE 0
#define EXPONENTIAL_CURVE 1
#define LOGARITHMIC_CURVE 2

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;
uniform vec4 color_in;
uniform int curve_in;

uniform float ove_tprog_all;
uniform float ove_tprog_out;
uniform float ove_tprog_in;

in vec2 ove_texcoord;
out vec4 frag_color;

float TransformCurve(float linear) {
    if (curve_in == EXPONENTIAL_CURVE) {
        return linear * linear;
    } else if (curve_in == LOGARITHMIC_CURVE) {
        return sqrt(linear);
    } else {
        return linear;
    }
}

void main(void) {
    if (out_block_in_enabled && in_block_in_enabled) {
        // mix(x, y , a): a(1-x) + b(x)
        vec4 out_block_col = mix(color_in, texture(out_block_in, ove_texcoord),TransformCurve(ove_tprog_out));
        vec4 in_block_col = mix(color_in, texture(in_block_in, ove_texcoord), TransformCurve(ove_tprog_in));

        frag_color = out_block_col + in_block_col;
    } else if (out_block_in_enabled) {
        frag_color = mix(color_in, texture(out_block_in, ove_texcoord), TransformCurve(ove_tprog_out));
    } else if (in_block_in_enabled) {
        frag_color = mix(texture(in_block_in, ove_texcoord), color_in, TransformCurve(1.0 - ove_tprog_in));
    } else {
        frag_color = vec4(0.0);
    }
}
