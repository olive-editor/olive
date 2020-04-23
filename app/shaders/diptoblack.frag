#version 150

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;

uniform float ove_tprog_out;
uniform float ove_tprog_in;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 out_block_col;
    vec4 in_block_col;

    if (out_block_in_enabled) {
        out_block_col = texture(out_block_in, ove_texcoord) * pow(ove_tprog_out, 2.0);
    } else {
        out_block_col = vec4(0.0);
    }

    if (in_block_in_enabled) {
        in_block_col = texture(in_block_in, ove_texcoord) * pow(ove_tprog_in, 2.0);
    } else {
        in_block_col = vec4(0.0);
    }

    fragColor = out_block_col + in_block_col;
}
