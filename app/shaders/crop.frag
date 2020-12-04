// Input variables
uniform sampler2D tex_in;
uniform float left_in;
uniform float top_in;
uniform float right_in;
uniform float bottom_in;
uniform float feather_in;
uniform vec2 resolution_in;

// Input texture coordinate
in vec2 ove_texcoord;

// Output color
out vec4 fragColor;

void main() {
    float multiplier = 1.0;

    vec2 feather_normalized = vec2(feather_in / resolution_in.x, feather_in / resolution_in.y);
    vec2 feather_normalized_half = feather_normalized * 0.5;

    // Calculate left cropping
    float left_adjustment;
    float right_adjustment;
    float top_adjustment;
    float bottom_adjustment;

    if (feather_in == 0.0) {
        if (ove_texcoord.x < left_in
            || ove_texcoord.x > (1.0-right_in)
            || ove_texcoord.y < (top_in)
            || ove_texcoord.y > (1.0-bottom_in)) {
            multiplier = 0.0;
        }
    } else {
        float left_adjustment = clamp((ove_texcoord.x - (left_in - feather_normalized.x*(1.0-left_in))) / feather_normalized.x, 0.0, 1.0);
        multiplier *= left_adjustment;

        float right_adjustment = 1.0-clamp((ove_texcoord.x - ((1.0-right_in) - feather_normalized.x*(right_in))) / feather_normalized.x, 0.0, 1.0);
        multiplier *= right_adjustment;

        float top_adjustment = clamp((ove_texcoord.y - (top_in - feather_normalized.y*(1.0-top_in))) / feather_normalized.y, 0.0, 1.0);
        multiplier *= top_adjustment;

        float bottom_adjustment = 1.0-clamp((ove_texcoord.y - ((1.0-bottom_in) - feather_normalized.y*(bottom_in))) / feather_normalized.y, 0.0, 1.0);
        multiplier *= bottom_adjustment;
    }

    if (multiplier > 0.0) {
        vec4 color = texture(tex_in, ove_texcoord) * multiplier;
        fragColor = color;
    } else {
        fragColor = vec4(0.0);
    }
}
