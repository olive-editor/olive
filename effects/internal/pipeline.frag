#ifdef GL_ES
precision mediump int;
precision mediump float;
#endif

uniform sampler2D texture;
uniform float opacity;
varying vec2 v_texcoord;

void main() {
  vec4 color = texture2D(texture, v_texcoord)*opacity;
  gl_FragColor = color;
}