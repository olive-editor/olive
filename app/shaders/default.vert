uniform mat4 ove_mvpmat;

attribute vec4 a_position;
attribute vec2 a_texcoord;

varying vec2 ove_texcoord;

void main() {
    gl_Position = ove_mvpmat * a_position;
    ove_texcoord = a_texcoord;
}
