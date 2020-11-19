#define LINEAR_CURVE 0
#define EXPONENTIAL_CURVE 1
#define LOGARITHMIC_CURVE 2

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;
uniform int curve_in;

uniform float ove_tprog_all;

in vec2 ove_texcoord;

out vec4 fragColor;

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
    vec4 composite = vec4(0.0);

    if (out_block_in_enabled) {
        composite += texture(out_block_in, ove_texcoord) * TransformCurve(1.0 - ove_tprog_all);
    }

    if (in_block_in_enabled) {
        composite += texture(in_block_in, ove_texcoord) * TransformCurve(ove_tprog_all);
    }

    fragColor = composite;
}
