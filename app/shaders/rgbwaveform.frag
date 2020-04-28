// Adapted from "RGB Waveform" by lebek
// https://www.shadertoy.com/view/4dK3Wc

#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;

uniform float threshold;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec3 col = vec3(0.0);
    // Set an increment default to 10 bit encodings. This would likely be
    // better served as a UI control, as waveforms will change their combing
    // based on how granular the increment is set. For example, it can be
    // challenging to spot 8 bit combing with an increment of 1. / 2.^8 - 1.
    float increment = 1.0 / (pow(2, 10) - 1.0);
    float maxb = ove_texcoord.y + increment;
    float minb = ove_texcoord.y - increment;

    // Intensity would make sense to also expose via the UI, as a density
    // slider allows you to peek past certain values or reveal very low
    // values. Hard coding it for now, as there isn't a clear way to have
    // the various bit depth / code values always display at a consistent
    // emission output strength.
    float intensity = 0.10;

    int y_lim = int(ove_resolution.y);

    vec3 cur_col = vec3(0.0);
    for (int i = 0; i < y_lim; i++) {
        cur_col = texture2D(
            ove_maintex,
            vec2(ove_texcoord.x, float(i) / float(ove_resolution.y))
        ).rgb;

        col += step(vec3(ove_texcoord.y - increment), cur_col) *
            step(cur_col, vec3(ove_texcoord.y + increment)) * intensity;

        // TODO: Implement proper luma / luminance based values.
        // This is simply taking the luminance weights derived from the
        // OCIO configuration or the RGB to XYZ matrix and multiplying
        // the above step approach by the luminance weights.
        // EG:
        // cur_lum = cur_col * lum_coeffs;
        //
        // col += step(vec3(ove_texcoord.y - increment), cur_lum) *
        //     step(cur_lum, vec3(ove_texcoord.y + increment));

    }

    gl_FragColor = vec4(col, 1.0);
}
