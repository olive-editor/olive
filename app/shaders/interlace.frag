uniform sampler2D top_tex_in;
uniform sampler2D bottom_tex_in;
uniform vec2 resolution_in;

in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
    float y_pixel = floor(ove_texcoord.y * resolution_in.y);

    if (mod(y_pixel, 2.0) == 0.0) {
      frag_color = texture(top_tex_in, ove_texcoord);
    } else {
      frag_color = texture(bottom_tex_in, ove_texcoord);
    }
}
