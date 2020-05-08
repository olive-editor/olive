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
    // float maxb = waveform_dims.y + increment;
    // float minb = waveform_dims.y - increment;

    float quantisation = 1.0 / ove_resolution.y;

    // int y_lim = int(waveform_dims.y);

    vec3 col = vec3(0.0);
    vec3 sum = vec3(0.0);
    float total_pixels = ove_resolution.x * ove_resolution.y;
    vec3 histogram_ratio = vec3(0.0);
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

    histogram_ratio = pow(sum / total_pixels, vec3(1.0 / 4.0));
    col = step(vec3(ove_texcoord.y + quantisation),
        histogram_ratio);

    fragColor = vec4(col, 0.8);
}
