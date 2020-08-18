#version 150

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;
uniform vec4 color_in;

uniform float ove_tprog_all;
uniform float ove_tprog_out;
uniform float ove_tprog_in;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    if (out_block_in_enabled && in_block_in_enabled) {
        vec4 out_block_col = mix(texture(out_block_in, ove_texcoord), color_in, ove_tprog_out);
        vec4 in_block_col = mix(texture(in_block_in, ove_texcoord), color_in, 1.0 - ove_tprog_in);

        fragColor = out_block_col + in_block_col;
    } else if (out_block_in_enabled) {
        fragColor = mix(texture(out_block_in, ove_texcoord), color_in, ove_tprog_all);
    } else if (in_block_in_enabled) {
        fragColor = mix(texture(in_block_in, ove_texcoord), color_in, 1.0 - ove_tprog_all);
    } else {
        fragColor = vec4(0.0);
    }
}
