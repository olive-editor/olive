#version 110

uniform vec2 ove_resolution;
varying vec2 ove_texcoord;
uniform int ove_iteration;

uniform sampler2D tex_in;
uniform float radius_in;
uniform bool horiz_in;
uniform bool vert_in;
uniform bool repeat_edge_pixels_in;

void main(void) {
    if (radius_in == 0.0
        || (ove_iteration == 0 && !horiz_in)
        || (ove_iteration == 1 && !vert_in)) {
        gl_FragColor = texture2D(tex_in, ove_texcoord);
        return;
    }
    
    // We only sample on hard pixels, so we don't accept decimal radii
    float real_radius = ceil(radius_in);

    // Calculate the weight of each pixel based on the radius
    float divider = 1.0 / real_radius;      

    vec4 composite = vec4(0.0);
    for (float i=-real_radius+0.5;i<=real_radius;i+=2.0) {
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
            composite += texture2D(tex_in, pixel_coord) * divider;
        }
    }
    gl_FragColor = composite;
}
