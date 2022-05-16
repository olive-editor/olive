uniform sampler2D tex_in;
uniform vec4 color_in;
uniform float distance_in;
uniform float angle_in;
uniform float radius_in;
uniform float opacity_in;
uniform vec2 resolution_in;
uniform sampler2D previous_iteration_in;
uniform bool fast_in;

uniform int ove_iteration;

in vec2 ove_texcoord;
out vec4 frag_color;

// Gaussian function uses PI
#define M_PI 3.1415926535897932384626433832795

// Single gaussian formula (unused, mainly here for documentation/just in case)
//float gaussian(float x, float sigma) {
//    return (1.0/(sigma*sqrt(2.0*M_PI)))*exp(-0.5*pow(x/sigma, 2.0));
//}

// Double gaussian formula, actually used in the code below
// Should be faster than the single gaussian above since it doesn't need sqrt()
float gaussian2(float x, float y, float sigma) {
  return (1.0/((sigma*sigma)*2.0*M_PI))*exp(-0.5*(((x*x) + (y*y))/(sigma*sigma)));
}

void main(void) {
  if (ove_iteration == 2 || radius_in == 0.0) {

    // Merge step
    vec4 composite = texture(tex_in, ove_texcoord);
    if (composite.a < 1.0) {
      // Convert degrees to radians
      float shadow_angle = ((angle_in + 90.0)*M_PI)/180.0;

      vec2 shadow_offset = vec2(cos(shadow_angle) * distance_in, sin(shadow_angle) * distance_in);
      shadow_offset /= resolution_in;
      shadow_offset += ove_texcoord;

      vec4 shadow_color = texture(previous_iteration_in, shadow_offset);

      shadow_color.rgb = color_in.rgb * shadow_color.a;
      shadow_color *= 1.0 - composite.a;
      shadow_color *= opacity_in;

      composite += shadow_color;
    }
    frag_color = composite;

  } else {
    // We only sample on hard pixels, so we don't accept decimal radii
    float real_radius = ceil(radius_in);

    vec4 composite = vec4(0.0);

    float divider, sigma;

    if (fast_in) {

      // Calculate the weight of each pixel based on the radius
      divider = 1.0 / real_radius;

    } else {

      // Using (radius = 3 * sigma) because 3 standard deviations covers 97% of the blur according to this document:
      // http://chemaguerra.com/gaussian-filter-radius/
      sigma = real_radius;
      real_radius *= 3.0;

      // Use gaussian formula to calculate the weight of all pixels
      divider = 0.0;
      for (float i = -real_radius + 0.5; i <= real_radius; i += 2.0) {
        divider += gaussian2(i, 0.0, sigma);
      }

    }

    for (float i = -real_radius + 0.5; i <= real_radius; i += 2.0) {
      float weight;

      if (fast_in) {
        weight = divider;
      } else {
        weight = gaussian2(i, 0.0, sigma) / divider;
      }

      vec2 pixel_coord = ove_texcoord;
      vec4 tex_col;
      if (ove_iteration == 0) {
        pixel_coord.x += i / resolution_in.x;

        // Pull from main texture
        tex_col = texture(tex_in, pixel_coord);
      } else if (ove_iteration == 1) {
        pixel_coord.y += i / resolution_in.y;

        // Pull from previous iteration
        tex_col = texture(previous_iteration_in, pixel_coord);
      }

      composite += tex_col * weight;
    }

    frag_color = composite;
  }

}
