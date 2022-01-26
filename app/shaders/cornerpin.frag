// Input texture
uniform sampler2D ove_maintex;
uniform sampler2D tex_in;

// Input texture coordinate
varying vec2 ove_texcoord;

void main() {
    vec4 color = texture2D(tex_in, ove_texcoord);
    gl_FragColor = color;
}