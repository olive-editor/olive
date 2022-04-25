uniform sampler2D tex_in;
uniform bool horiz_in;
uniform bool vert_in;

in vec2 ove_texcoord;
out vec4 frag_color;

void main(void) {
    if (!horiz_in && !vert_in) {
        frag_color = texture(tex_in, ove_texcoord);
        return;
    }

    vec2 new_coord = ove_texcoord;

    if (horiz_in) new_coord.x = 1.0 - new_coord.x;
    if (vert_in) new_coord.y = 1.0 - new_coord.y;

    frag_color = texture(tex_in, new_coord);
}
