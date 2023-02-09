uniform sampler2D ove_maintex;

uniform int interlacing;
uniform int pixel_height;

in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
  vec2 real_coord = ove_texcoord;
  if (interlacing != 0) {
    float field_height = float(pixel_height / 2);
    real_coord.y = floor(real_coord.y * field_height) + 0.25;
    if (interlacing == 2) {
      real_coord.y += 0.5;
    }
    real_coord.y /= field_height;
  }

  frag_color = texture(ove_maintex, real_coord);
}
