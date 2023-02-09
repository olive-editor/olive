// Inputs
uniform sampler2D tex_in;
uniform float opacity_in;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
  frag_color = texture(tex_in, ove_texcoord) * opacity_in;
}
