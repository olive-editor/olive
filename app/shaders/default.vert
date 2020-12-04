uniform mat4 ove_mvpmat;

in vec4 a_position;
in vec2 a_texcoord;

out vec2 ove_texcoord;

void main() {
    gl_Position = ove_mvpmat * a_position;
    ove_texcoord = a_texcoord;
}