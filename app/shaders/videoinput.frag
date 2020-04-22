#version 150

uniform sampler2D footage_in;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    fragColor = texture(footage_in, ove_texcoord);
}
