uniform sampler2D y_channel;
uniform sampler2D u_channel;
uniform sampler2D v_channel;

uniform int bits_per_pixel;

in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
  vec4 rgba;

  vec3 yuv;

  yuv.r = texture(y_channel, ove_texcoord).r;
  yuv.g = texture(u_channel, ove_texcoord).r;
  yuv.b = texture(v_channel, ove_texcoord).r;

  if (bits_per_pixel == 10) {
    yuv *= 64.0;
  } else if (bits_per_pixel == 12) {
    yuv *= 16.0;
  }

  yuv.r = 1.1643 * (yuv.r - 0.0625);
  yuv.g = yuv.g - 0.5;
  yuv.b = yuv.b - 0.5;

  rgba.r = yuv.r + 1.5958 * yuv.b;
  rgba.g = yuv.r - 0.39173 * yuv.g - 0.81290 * yuv.b;
  rgba.b = yuv.r + 2.017 * yuv.g;
  rgba.a = 1.0;

  frag_color = rgba;
}
