uniform sampler2D y_channel;
uniform sampler2D u_channel;
uniform sampler2D v_channel;

uniform int bits_per_pixel;
uniform bool full_range;

uniform float yuv_crv;
uniform float yuv_cgu;
uniform float yuv_cgv;
uniform float yuv_cbu;

in vec2 ove_texcoord;
out vec4 frag_color;

void main()
{
  // Sample YUV planes
  vec3 yuv;
  yuv.r = texture(y_channel, ove_texcoord).r;
  yuv.g = texture(u_channel, ove_texcoord).r;
  yuv.b = texture(v_channel, ove_texcoord).r;

  // Pixels will have come in aligned to 16-bit regardless of their actual bit depth, so they must
  // be scaled as if they were actually 16-bit
  if (bits_per_pixel == 8) {
    // Convert 0.0-1.0 to -0.5-0.5
    yuv.gb -= (128.0/255.0);
  } else if (bits_per_pixel == 10) {
    // Scale from 10-bit to 16-bit
    yuv *= (65535.0/1023.0);

    // Convert 0.0-1.0 to -0.5-0.5
    yuv.gb -= (512.0/1023.0);
  } else if (bits_per_pixel == 12) {
    // Scale from 12-bit to 16-bit
    yuv *= (65535.0/4095.0);

    // Convert 0.0-1.0 to -0.5-0.5
    yuv.gb -= (2048.0/4095.0);
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

  frag_color = rgba;
}
