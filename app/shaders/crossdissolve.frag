#version 110

varying vec2 ove_texcoord;

uniform sampler2D out_block_in;
uniform sampler2D in_block_in;
uniform bool out_block_in_enabled;
uniform bool in_block_in_enabled;

uniform float ove_tprog_all;

void main(void) {
  vec4 composite = vec4(0.0);

  if (out_block_in_enabled) {
    composite += texture2D(out_block_in, ove_texcoord) * (1.0 - ove_tprog_all);
  }

  if (in_block_in_enabled) {
    vec4 in_block_col = texture2D(in_block_in, ove_texcoord) * ove_tprog_all;
    composite *= 1.0 - in_block_col.a;
    composite += in_block_col;
  }
  
  gl_FragColor = composite;
}
