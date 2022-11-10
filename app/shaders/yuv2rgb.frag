uniform sampler2D y_channel;
uniform sampler2D u_channel;
uniform sampler2D v_channel;

uniform int bits_per_pixel;
uniform bool full_range;

uniform float yuv_crv;
uniform float yuv_cgu;
uniform float yuv_cgv;
uniform float yuv_cbu;

uniform int interlacing;
uniform int pixel_height;

in vec2 ove_texcoord;
out vec4 frag_color;

void main()
{
  vec2 real_coord = ove_texcoord;
  if (interlacing != 0) {
    float field_height = float(pixel_height / 2);
    real_coord.y = floor(real_coord.y * field_height) + 0.25;
    if (interlacing == 2) {
      real_coord.y += 0.5;
    }
    real_coord.y /= field_height;
  }

  // Sample YUV planes
  vec3 yuv;
  yuv.r = texture(y_channel, real_coord).r;
  yuv.g = texture(u_channel, real_coord).r;
  yuv.b = texture(v_channel, real_coord).r;

  // Pixels will have come in aligned to 16-bit regardless of their actual bit depth, so they must
  // be scaled as if they were actually 16-bit
  if (bits_per_pixel == 8) {
    // Convert 0.0-1.0 to -0.5-0.5
    yuv.gb -= (128.0/255.0);
  } else if (bits_per_pixel == 10) {
    // Convert 0.0-1.0 to -0.5-0.5
    yuv.gb -= (512.0/1023.0);

    yuv *= 64.0;
  } else if (bits_per_pixel == 12) {
    // Convert 0.0-1.0 to -0.5-0.5
    yuv.gb -= (2048.0/4095.0);

    yuv *= 16.0;
  }

  // Convert YUV limited range from 16-235 to 0-255
  yuv.r -= 0.0625; // 16/256
  yuv.r *= 1.1643; // 255/219

  // Use coefficients to weigh YUV into RGB
  vec4 rgba;
  rgba.r = yuv.r + yuv_crv * yuv.b;
  rgba.g = yuv.r - yuv_cgu * yuv.g - yuv_cgv * yuv.b;
  rgba.b = yuv.r + yuv_cbu * yuv.g;

  // If the expected value is full range, transform to full range here
  if (full_range) {
    rgba.rgb /= 1.1643;
    rgba.rgb += 0.0625;
  }

  // Currently this shader is only used for RGB textures, so just set alpha to 1
  rgba.a = 1.0;

  rgba.r = texture(u_channel, real_coord).r;
  rgba.g = 0.5;
  rgba.b = 128.0/255.0;

  frag_color = rgba;
}
