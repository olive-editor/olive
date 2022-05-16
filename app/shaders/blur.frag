uniform sampler2D tex_in;
uniform int method_in;
uniform float radius_in;
uniform bool horiz_in;
uniform bool vert_in;
uniform bool repeat_edge_pixels_in;
uniform vec2 resolution_in;

// Directional
uniform float directional_degrees_in;

// Radial
uniform vec2 radial_center_in;

uniform int ove_iteration;

in vec2 ove_texcoord;
out vec4 frag_color;

// Gaussian function uses PI
#define M_PI 3.1415926535897932384626433832795

// Methods
#define METHOD_BOX_BLUR 0
#define METHOD_GAUSSIAN_BLUR 1
#define METHOD_DIRECTIONAL_BLUR 2
#define METHOD_RADIAL_BLUR 3

// Mode
#define MODE_NONE 0
#define MODE_HORIZONTAL 1
#define MODE_VERTICAL 2

// Single gaussian formula (unused, mainly here for documentation/just in case)
//float gaussian(float x, float sigma) {
//    return (1.0/(sigma*sqrt(2.0*M_PI)))*exp(-0.5*pow(x/sigma, 2.0));
//}

// Double gaussian formula, actually used in the code below
// Should be faster than the single gaussian above since it doesn't need sqrt()
float gaussian2(float x, float y, float sigma) {
    return (1.0/((sigma*sigma)*2.0*M_PI))*exp(-0.5*(((x*x) + (y*y))/(sigma*sigma)));
}

int determine_mode() {
    if (radius_in == 0.0) {
        return MODE_NONE;
    }

    if (!horiz_in && !vert_in) {
        return MODE_NONE;
    }

    if (horiz_in && !vert_in) {
        return MODE_HORIZONTAL;
    }

    if (vert_in && !horiz_in) {
        return MODE_VERTICAL;
    }

    if (ove_iteration == 0) {
        return MODE_HORIZONTAL;
    }

    if (ove_iteration == 1) {
        return MODE_VERTICAL;
    }
}

vec4 add_to_composite(vec4 composite, vec2 pixel_coord, float weight)
{
  if (repeat_edge_pixels_in
      || (pixel_coord.x >= 0.0
          && pixel_coord.x < 1.0
          && pixel_coord.y >= 0.0
          && pixel_coord.y < 1.0)) {
      composite += texture(tex_in, pixel_coord) * weight;
  }

  return composite;
}

void main(void) {
    int mode = determine_mode();

    if (mode == MODE_NONE) {
        frag_color = texture(tex_in, ove_texcoord);
        return;
    }

    // We only sample on hard pixels, so we don't accept decimal radii
    float real_radius = ceil(radius_in);

    vec4 composite = vec4(0.0);

    float divider, sigma;

    if (method_in == METHOD_DIRECTIONAL_BLUR || method_in == METHOD_RADIAL_BLUR) {
      // Despite similar math, these are lighter methods perceptually, so we double the radius to
      // better match box/gaussian
      real_radius *= 2.0;
    }

    if (method_in == METHOD_BOX_BLUR || method_in == METHOD_DIRECTIONAL_BLUR) {

        // Calculate the weight of each pixel based on the radius
        divider = 1.0 / real_radius;

    } else if (method_in == METHOD_GAUSSIAN_BLUR) {

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

    if (method_in == METHOD_BOX_BLUR || method_in == METHOD_GAUSSIAN_BLUR) {
        for (float i = -real_radius + 0.5; i <= real_radius; i += 2.0) {
            float weight;

            if (method_in == METHOD_BOX_BLUR) {
                weight = divider;
            } else if (method_in == METHOD_GAUSSIAN_BLUR) {
                weight = gaussian2(i, 0.0, sigma) / divider;
            }

            vec2 pixel_coord = ove_texcoord;
            if (mode == MODE_HORIZONTAL) {
                pixel_coord.x += i / resolution_in.x;
            } else if (mode == MODE_VERTICAL) {
                pixel_coord.y += i / resolution_in.y;
            }

            composite = add_to_composite(composite, pixel_coord, weight);
        }
    } else if (method_in == METHOD_DIRECTIONAL_BLUR || method_in == METHOD_RADIAL_BLUR) {
        float angle;

        if (method_in == METHOD_DIRECTIONAL_BLUR) {
          // Convert directional degrees to radians
          angle = (directional_degrees_in*M_PI)/180.0;
        } else {
          // Calculate angle from distance of center to current coordinate
          vec2 distance = (ove_texcoord - 0.5) * (resolution_in) - radial_center_in;
          angle = atan(distance.y/distance.x);

          float multiplier = length(distance) / resolution_in.y * 2.0;

          real_radius = ceil(radius_in * multiplier);
          divider = 1.0 / real_radius;
        }

        // Get angles
        float sin_angle = sin(angle);
        float cos_angle = cos(angle);

        for (float i = -real_radius + 0.5; i <= real_radius; i += 2.0) {
          vec2 pixel_coord = ove_texcoord;

          pixel_coord.y += sin_angle * i / resolution_in.y;
          pixel_coord.x += cos_angle * i / resolution_in.x;

          composite = add_to_composite(composite, pixel_coord, divider);
        }
    }

    frag_color = composite;
}
