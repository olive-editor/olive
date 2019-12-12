#version 110

varying vec2 ove_texcoord;

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;

uniform float ove_tprog_out;
uniform float ove_tprog_in;

void main(void) {
  vec4 out_block_col;
  vec4 in_block_col;

  if (out_block_in_enabled) {
    out_block_col = texture2D(out_block_in, ove_texcoord) * pow(ove_tprog_out, 2.0);
  } else {
    out_block_col = vec4(0.0);
  }

  if (in_block_in_enabled) {
    in_block_col = texture2D(in_block_in, ove_texcoord) * pow(ove_tprog_in, 2.0);
  } else {
    in_block_col = vec4(0.0);
  }
  
  gl_FragColor = out_block_col + in_block_col;
}
