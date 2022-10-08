// Input texture
uniform sampler2D tex_a;
uniform sampler2D tex_b;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
    frag_color = texture(tex_a, ove_texcoord) * texture(tex_b, ove_texcoord);
}
