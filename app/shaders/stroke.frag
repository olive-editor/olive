// Node parameter inputs
uniform sampler2D tex_in;
uniform vec4 color_in;
uniform float radius_in;
uniform float opacity_in;
uniform bool inner_in;

// Standard inputs
uniform vec2 ove_resolution;
uniform int ove_iteration;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 pixel_here = texture(tex_in, ove_texcoord);

    // Detect no-op situations
    if (radius_in == 0.0
        || opacity_in == 0.0
        || (inner_in && pixel_here.a == 0.0)
        || (!inner_in && pixel_here.a == 1.0)) {
        // No-op, do nothing
        fragColor = pixel_here;
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
                float alpha = texture(tex_in, ove_texcoord + vec2(x_coord, y_coord)).a;

                if (inner_in) {
                    alpha = 1.0 - alpha;
                }

                stroke_weight += alpha;

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

    stroke_weight *= opacity_in;

    if (inner_in) {
        stroke_weight *= pixel_here.a;
    }

    // Make RGBA color
    vec4 stroke_col = color_in * stroke_weight;

    if (inner_in) {
        // Alpha over the stroke over the texture
        stroke_col = pixel_here * (1.0 - stroke_col.a) + stroke_col;
    } else {
        // Alpha over the texture over the stroke
        stroke_col = stroke_col * (1.0 - pixel_here.a) + pixel_here;
    }

    fragColor = stroke_col;
}
