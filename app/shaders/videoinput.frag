#version 110

varying vec2 ove_texcoord;
uniform sampler2D footage_in;

void main(void) {
  gl_FragColor = texture2D(footage_in, ove_texcoord);
}
