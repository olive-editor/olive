uniform sampler2D tex_in;
in vec2 ove_texcoord;
out vec4 frag_color;

%1

void main() {
  vec4 col = texture2D(tex_in, ove_texcoord);

  col = %2(col);

  frag_color = col;
}
