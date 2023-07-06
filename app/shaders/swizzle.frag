// Input
uniform sampler2D red_texture;
uniform bool red_texture_enabled;
uniform sampler2D green_texture;
uniform bool green_texture_enabled;
uniform sampler2D blue_texture;
uniform bool blue_texture_enabled;
uniform sampler2D alpha_texture;
uniform bool alpha_texture_enabled;
uniform int red_channel;
uniform int green_channel;
uniform int blue_channel;
uniform int alpha_channel;

in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
  vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

  if (red_texture_enabled) {
    color.r = texture(red_texture, ove_texcoord)[red_channel];
  }

  if (green_texture_enabled) {
    color.g = texture(green_texture, ove_texcoord)[green_channel];
  }

  if (blue_texture_enabled) {
    color.b = texture(blue_texture, ove_texcoord)[blue_channel];
  }

  if (alpha_texture_enabled) {
    color.a = texture(alpha_texture, ove_texcoord)[alpha_channel];
  }

  frag_color = color;
}
