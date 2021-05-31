// Input texture
uniform sampler2D ove_maintex;

// Input texture coordinate
varying vec2 ove_texcoord;

void main() {
    vec4 color = texture2D(ove_maintex, ove_texcoord);
    gl_FragColor = color;
}
