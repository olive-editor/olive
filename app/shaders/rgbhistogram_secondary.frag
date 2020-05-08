#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;
// uniform vec3 luma_coeffs;

// uniform float waveform_scale;
// uniform vec2 waveform_dims;
uniform vec4 histogram_region;
// uniform vec4 waveform_uv;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    // Set an increment default to 10 bit encodings. This would likely be
    // better served as a UI control, as waveforms will change their combing
    // based on how granular the increment is set. For example, it can be
    // challenging to spot 8 bit combing with an increment of 1. / 2.^8 - 1.
    float increment = 1.0 / (pow(2, 10) - 1.0);
    // float maxb = waveform_dims.y + increment;
    // float minb = waveform_dims.y - increment;

    // Intensity would make sense to also expose via the UI, as a density
    // slider allows you to peek past certain values or reveal very low
    // values. Hard coding it for now, as there isn't a clear way to have
    // the various bit depth / code values always display at a consistent
    // emission output strength.
    // float intensity = 0.10;

    // int y_lim = int(waveform_dims.y);

    vec3 col = vec3(0.0);
    vec3 sum = vec3(0.0);
    vec3 cur_col = vec3(0.0);
    // vec3 cur_lum = vec3(0.0);

    // if (
    //     (gl_FragCoord.x >= histogram_region.x) &&
    //     (gl_FragCoord.y >= histogram_region.y) &&
    //     (gl_FragCoord.x < histogram_region.z) &&
    //     (gl_FragCoord.y < histogram_region.w)
    // ) {
    //     col = vec3(0.7, 0.0, 0.7);
    // }
    //     // int start = int(waveform_region.y);
    //     int stop = int(waveform_dims.y);
        float ratio = 0.0;
    //     float waveform_x = (ove_texcoord.x - waveform_uv.x) / waveform_scale;
    //     float waveform_y = (ove_texcoord.y - waveform_uv.y) / waveform_scale;
        for (int i = 0; i < ove_viewport.y; i++) {
            ratio = float(i) / float(ove_viewport.y - 1);
            sum += texture(
                ove_maintex,
                vec2(ove_texcoord.x, ratio)
            ).rgb;
        }
    // }
    col = step(vec3(ove_texcoord.y + increment), sum / vec3(ove_viewport.y));

    col = texture(ove_maintex, vec2(ove_texcoord)).rgb;
    fragColor = vec4(col, 1.0);
}
