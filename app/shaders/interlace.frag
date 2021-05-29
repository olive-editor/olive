uniform sampler2D top_tex_in;
uniform sampler2D bottom_tex_in;
uniform vec2 resolution_in;

varying vec2 ove_texcoord;

void main() {
    float y_pixel = floor(ove_texcoord.y * resolution_in.y);

    if (mod(y_pixel, 2.0) == 0.0) {
      gl_FragColor = texture2D(top_tex_in, ove_texcoord);
    } else {
      gl_FragColor = texture2D(bottom_tex_in, ove_texcoord);
    }
}
