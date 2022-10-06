// Input texture
uniform sampler2D tex_in;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
    vec4 color = texture(tex_in, ove_texcoord);
    color.rgb = 1.0 - color.rgb;
    frag_color = color;
}
