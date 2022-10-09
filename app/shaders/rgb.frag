// Input texture
uniform sampler2D texture_in;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

// Input color
uniform vec4 color_in;

void main() {
  vec4 color = texture(texture_in, ove_texcoord);
  color.rgb = color_in.rgb * color.a;
  frag_color = color;
}
