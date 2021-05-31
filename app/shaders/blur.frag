uniform sampler2D tex_in;
uniform int method_in;
uniform float radius_in;
uniform bool horiz_in;
uniform bool vert_in;
uniform bool repeat_edge_pixels_in;
uniform vec2 resolution_in;

uniform int ove_iteration;

varying vec2 ove_texcoord;

// Gaussian function uses PI
#define M_PI 3.1415926535897932384626433832795

// Methods
#define METHOD_BOX_BLUR 0
#define METHOD_GAUSSIAN_BLUR 1

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

void main(void) {
    int mode = determine_mode();

    if (mode == MODE_NONE) {
        gl_FragColor = texture2D(tex_in, ove_texcoord);
        return;
    }

    // We only sample on hard pixels, so we don't accept decimal radii
    float real_radius = ceil(radius_in);

    vec4 composite = vec4(0.0);

    float divider, sigma;

    if (method_in == METHOD_BOX_BLUR) {

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
