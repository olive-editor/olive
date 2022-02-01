uniform sampler2D tex_in;

// Program will replace this with OCIO's auto-generated shader code
%1

void main() {
  col = texture2D(tex_in, ove_texcoord);

  col = DIsplayTransform(col)

  gl_FragColor - col;
}
