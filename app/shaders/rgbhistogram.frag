// Adapted from "RGB Waveform" by lebek
// https://www.shadertoy.com/view/4dK3Wc

#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;
// uniform vec3 luma_coeffs;

// uniform float waveform_scale;
// uniform vec2 waveform_dims;
// uniform vec4 waveform_region;
// uniform vec4 waveform_uv;

// in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec3 col = vec3(0.0);
    // Set an increment default to 10 bit encodings. As per waveform,
    // the histogram increment should likely be based on a UI control. The
    // value "binning" can be more or less challenging to spot based on
    // things like resolution of the histogram etc. Here we are simply
    // assuming 10 bit bins.
    float increment = 1.0 / (pow(2, 10) - 1.0);

    // For the histogram, the y dimension we are pumping the values into
    // will end up representing the overall bin count. This is based
    // on above. Here we need to set thresholds for when a value falls
    // into a single bin. This is unlike traditional histograms that have
    // the privilege of using the actual code values, as we are in a
    // float based pipeline.
    float maxb = waveform_dims.y + increment;
    float minb = waveform_dims.y - increment;

    // The "intensity" here is our basic increment value that indicates we
    // have a value in the bin. We will use the ratio of X width here. Why?
    // Because the histogram's binning is based on the total count of values
    // at a given position, which in our waveform, is the Y position, where
    // X represents the count given the scalings.
    float intensity = 1.0 / ove_resolution.x;

    // This is the looping value for grabbing a column. Here we will use
    // the destination height to maximize our counting.
    int y_lim = int(ove_resolution).y);

    // Variables to store the current colour sum and the luminance / luma
    // weight sum.
    vec3 cur_col = vec3(0.0);
    vec3 cur_lum = vec3(0.0);

    // Ratio will be used to determine how far along the column we are.
    float ratio = 0.0;


    float waveform_x = (ove_texcoord.x - waveform_uv.x) / waveform_scale;
    float waveform_y = (ove_texcoord.y - waveform_uv.y) / waveform_scale;
    for (int i = 0; i < waveform_dims.y; i++) {
        ratio = float(i) / float(waveform_dims.y - 1);
        cur_col = texture(
            ove_maintex,
            vec2(waveform_x, ratio)
        ).rgb;

        col += step(vec3(waveform_y - increment), cur_col) *
            step(cur_col, vec3(waveform_y + increment)) * intensity;

        cur_lum = vec3(dot(cur_col, luma_coeffs));

        col += step(vec3(waveform_y - increment), cur_lum) *
            step(cur_lum, vec3(waveform_y + increment)) * intensity;
    }

    fragColor = vec4(vec3(0.7, 0.7, 0.0), 1.0);
}
