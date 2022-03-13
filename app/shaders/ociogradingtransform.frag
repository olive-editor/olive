uniform sampler2D tex_in;
varying vec2 ove_texcoord;

// Program will replace this with OCIO's auto-generated shader code
%1

void main() {
  vec4 col = texture2D(tex_in, ove_texcoord);

  col = OCIOGradingTransform(col);

  gl_FragColor = col;
}
