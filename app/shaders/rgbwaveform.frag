// Adapted from "RGB Waveform" by lebek
// https://www.shadertoy.com/view/4dK3Wc

#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;
uniform vec3 luma_coeffs;
uniform vec4 channel_swizzle;

uniform float waveform_scale;
uniform vec2 waveform_dims;
uniform vec4 waveform_region;
uniform vec4 waveform_uv;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec3 col = vec3(0.0);
    // Set an increment default to 10 bit encodings. This would likely be
    // better served as a UI control, as waveforms will change their combing
    // based on how granular the increment is set. For example, it can be
    // challenging to spot 8 bit combing with an increment of 1. / 2.^8 - 1.
    float increment = 1.0 / (pow(2, 10) - 1.0);
    float maxb = waveform_dims.y + increment;
    float minb = waveform_dims.y - increment;

    // Intensity would make sense to also expose via the UI, as a density
    // slider allows you to peek past certain values or reveal very low
    // values. Hard coding it for now, as there isn't a clear way to have
    // the various bit depth / code values always display at a consistent
    // emission output strength.
    float intensity = 0.10;

    int y_lim = int(waveform_dims.y);

    vec3 cur_col = vec3(0.0);
    vec3 cur_lum = vec3(0.0);

    if (
        (gl_FragCoord.x >= waveform_region.x) &&
        (gl_FragCoord.y >= waveform_region.y) &&
        (gl_FragCoord.x < waveform_region.z) &&
        (gl_FragCoord.y < waveform_region.w)
    ) {
        // col = vec3(0.5, 0.5, 0.0);
        // int start = int(waveform_region.y);
        int stop = int(waveform_dims.y);
        float ratio = 0.0;
        float waveform_x = (ove_texcoord.x - waveform_uv.x) / waveform_scale;
        float waveform_y = (ove_texcoord.y - waveform_uv.y) / waveform_scale;
        float step_below = waveform_y - increment;
        float step_above = waveform_y + increment;
        float ratio_divisor =  1.0 / float(waveform_dims.y - 1);

        vec3 step_below_vec = vec3(waveform_y - increment);
        vec3 step_above_vec = vec3(waveform_y + increment);

        for (int i = 0; i < waveform_dims.y; i++) {
            ratio = float(i) / float(waveform_dims.y - 1);
            cur_col = texture(
                ove_maintex,
                vec2(waveform_x, ratio)
            ).rgb;

            if(channel_swizzle.r == 1){
                col.r += step(step_below, cur_col.r) *
                      step(cur_col.r, step_above) * intensity;
            }

            if(channel_swizzle.g == 1){
                col.g += step(step_below, cur_col.g) *
                      step(cur_col.g, step_above) * intensity;
            }

            if(channel_swizzle.b == 1){
                col.b += step(step_below, cur_col.b) *
                      step(cur_col.b, step_above) * intensity;
            }

            if(channel_swizzle.w == 1){
                cur_lum = vec3(dot(cur_col, luma_coeffs));

                col += step(step_below_vec, cur_lum) *
                    step(cur_lum, step_above_vec) * intensity;
            }
        }
    }

    fragColor = vec4(col, 1.0);
}
