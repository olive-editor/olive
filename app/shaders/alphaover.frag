#version 110

varying vec2 ove_texcoord;

uniform sampler2D base_in;
uniform sampler2D blend_in;

void main(void) {
  vec4 base_col = texture2D(base_in, ove_texcoord);
  vec4 blend_col = texture2D(blend_in, ove_texcoord);
  base_col *= 1.0 - blend_col.a;
  base_col += blend_col;
  
  gl_FragColor = base_col;
}
