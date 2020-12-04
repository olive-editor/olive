// Input texture
uniform sampler2D ove_maintex;

// Input texture coordinate
in vec2 ove_texcoord;

// Output color
out vec4 fragColor;

void main() {
    vec4 color = texture(ove_maintex, ove_texcoord);
    fragColor = color;
}
