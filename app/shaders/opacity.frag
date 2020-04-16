#version 110

varying vec2 ove_texcoord;

uniform sampler2D tex_in;
uniform float opacity_in;

void main(void) {
  gl_FragColor = texture2D(tex_in, ove_texcoord) * (opacity_in);
}
