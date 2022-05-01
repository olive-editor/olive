// Input texture
uniform sampler2D ove_maintex;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
    vec4 color = texture(ove_maintex, ove_texcoord);
    frag_color = color;
}
