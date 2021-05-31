uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;
uniform vec4 color_in;

uniform float ove_tprog_all;
uniform float ove_tprog_out;
uniform float ove_tprog_in;

varying vec2 ove_texcoord;

void main(void) {
    if (out_block_in_enabled && in_block_in_enabled) {
        vec4 out_block_col = mix(texture2D(out_block_in, ove_texcoord), color_in, ove_tprog_out);
        vec4 in_block_col = mix(texture2D(in_block_in, ove_texcoord), color_in, 1.0 - ove_tprog_in);

        gl_FragColor = out_block_col + in_block_col;
    } else if (out_block_in_enabled) {
        gl_FragColor = mix(texture2D(out_block_in, ove_texcoord), color_in, ove_tprog_all);
    } else if (in_block_in_enabled) {
        gl_FragColor = mix(texture2D(in_block_in, ove_texcoord), color_in, 1.0 - ove_tprog_all);
    } else {
        gl_FragColor = vec4(0.0);
    }
}
