#version 150

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;

uniform float ove_tprog_all;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 composite = vec4(0.0);

    if (out_block_in_enabled) {
        composite += texture(out_block_in, ove_texcoord) * (1.0 - ove_tprog_all);
    }

    if (in_block_in_enabled) {
        vec4 in_block_col = texture(in_block_in, ove_texcoord) * ove_tprog_all;
        composite += in_block_col;
    }

    fragColor = composite;
}
