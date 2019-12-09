#version 110

varying vec2 ove_tex_coord;

uniform sampler2D tex_in;
uniform float opacity_in;

void main(void) {
  gl_FragColor = texture2D(tex_in, ove_tex_coord) * (opacity_in * 0.01);
}
