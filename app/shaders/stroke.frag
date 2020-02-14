#version 110

// Standard inputs
uniform vec2 ove_resolution;
varying vec2 ove_texcoord;
uniform int ove_iteration;

// Node parameter inputs
uniform sampler2D tex_in;
uniform vec3 color_in;
uniform float radius_in;
uniform float opacity_in;

void main(void) {
    if (radius_in == 0.0 || opacity_in == 0.0) {
        // No-op, do nothing
        gl_FragColor = texture2D(tex_in, ove_texcoord);
        return;
    }

    float radius = ceil(radius_in);

    float stroke_weight = 0.0;

    // Loop over box
    for (float i=-radius + 0.5; i<=radius; i += 2.0) {
        float x_coord = i / ove_resolution.x;

        for (float j=-radius + 0.5; j<=radius; j += 2.0) {
            float y_coord = j / ove_resolution.y;

            if (abs(length(vec2(i, j))) < radius) {
                // Get pixel here
                stroke_weight += texture2D(tex_in, ove_texcoord + vec2(x_coord, y_coord)).a;

                if (stroke_weight >= 1.0) {
                    break;
                }
            }
        }

        if (stroke_weight >= 1.0) {
            stroke_weight = 1.0;
            break;
        }
    }

    stroke_weight *= opacity_in * 0.01;

    // Make RGBA color
    vec4 stroke_col = vec4(vec3(1.0) * stroke_weight, stroke_weight);
    //vec4 stroke_col = vec4(color_in * stroke_weight, stroke_weight);

    // Alpha over color here
    vec4 pixel_here = texture2D(tex_in, ove_texcoord);

    stroke_col *= 1.0 - pixel_here.a;
    stroke_col += pixel_here;

    gl_FragColor = stroke_col;
}
