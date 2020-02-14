#version 110

// Standard inputs
uniform vec2 ove_resolution;
varying vec2 ove_texcoord;
uniform int ove_iteration;

// Node parameter inputs
uniform sampler2D tex_in;
uniform float sigma_in;
uniform bool horiz_in;
uniform bool vert_in;
uniform bool repeat_edge_pixels_in;

// Gaussian function uses PI
#define M_PI 3.1415926535897932384626433832795

// Single gaussian formula (unused, mainly here for documentation/just in case)
//float gaussian(float x, float sigma) {
//    return (1.0/(sigma*sqrt(2.0*M_PI)))*exp(-0.5*pow(x/sigma, 2.0));
//}

// Double gaussian formula, actually used in the code below
// Should be faster than the single gaussian above since it doesn't need sqrt()
float gaussian2(float x, float y, float sigma) {
    return (1.0/(pow(sigma, 2.0)*2.0*M_PI))*exp(-0.5*((pow(x, 2.0) + pow(y, 2.0))/pow(sigma, 2.0)));
}

void main(void) {
    // Determine this iteration should process anything or not
    if (sigma_in == 0.0                          // If the radius is zero
        || (ove_iteration == 0 && !horiz_in)     // Or this iteration (horizontal) is disabled
        || (ove_iteration == 1 && !vert_in)) {   // Or this iteration (vertical) is disabled
        // No-op, do nothing
        gl_FragColor = texture2D(tex_in, ove_texcoord);
        return;
    }

    // Using (3 * sigma) because 3 standard deviations covers 97% of the blur according to this document:
    // http://chemaguerra.com/gaussian-filter-radius/
    float radius = ceil(3.0 * sigma_in);

    // Use gaussian formula to calculate the weight of all pixels
    float sum = 0.0;
    for (float i=-radius+0.5;i<=radius;i+=2.0) {
        sum += gaussian2(i, 0.0, sigma_in);
    }

    // We can take advantage of OpenGL's linear interpolation by sampling between pixels. This produces a mathematically
    // identical result while allowing us to sample the texture half as many times.
    vec4 composite = vec4(0.0);
    for (float i=-radius+0.5;i<=radius;i+=2.0) {
        // Calculate weight of this pixel using gaussian
        float weight = (gaussian2(i, 0.0, sigma_in)/sum);

        // Determine pixel coordinate based on iteration
        // We use two iterations horizontally and vertically since that produces a mathematically identical result and
        // (2*radius) is much faster than (radius^2)
        vec2 pixel_coord = ove_texcoord;
        if (ove_iteration == 0) {
            pixel_coord.x += i/ove_resolution.x;
        } else if (ove_iteration == 1) {
            pixel_coord.y += i/ove_resolution.y;
        }

        if (repeat_edge_pixels_in
            || (pixel_coord.x >= 0.0
                && pixel_coord.x < 1.0
                && pixel_coord.y >= 0.0
                && pixel_coord.y < 1.0)) {
            composite += texture2D(tex_in, pixel_coord) * weight;
        }
    }
    gl_FragColor = composite;
}
