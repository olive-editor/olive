#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;
uniform vec3 luma_coeffs;

uniform float waveform_scale;
uniform vec2 waveform_dims;
uniform vec4 waveform_region;
uniform vec4 waveform_region_uv;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 col = vec4(0.0);

    if (
        (gl_FragCoord.x >= waveform_region.x) &&
        (gl_FragCoord.y >= waveform_region.y) &&
        (gl_FragCoord.x < waveform_region.z) &&
        (gl_FragCoord.y < waveform_region.w)
    ) {
        float increment = 0.5;
        float uv_increment = increment / (ove_viewport.y - 1.0);
        float intensity = 0.10;
        vec4 cur_col = vec4(0.0);
        float ratio = 0.0;
        vec2 waveform_current_uv = vec2(
            (ove_texcoord.x - waveform_region_uv.x) / waveform_scale,
            (ove_texcoord.y - waveform_region_uv.y) / waveform_scale
        );

        for (int i = 0; i < waveform_dims.y; i++) {
            ratio = float(i) / float(waveform_dims.y - 1);
            cur_col.rgb = texture(
                ove_maintex,
                vec2(waveform_current_uv.x, ratio)
            ).rgb;

            cur_col.w = dot(cur_col.rgb, luma_coeffs);

            col += (
                step(vec4(waveform_current_uv.y - uv_increment), cur_col) *
                step(cur_col, vec4(waveform_current_uv.y + uv_increment)) *
                intensity) +
                (step(1.0 - uv_increment, waveform_current_uv.y) *
                step(vec4(1.0 - uv_increment), cur_col) * intensity);
        }
    }

    col.rgb += vec3(col.w);
    fragColor = vec4(col.rgb, 1.0);
}
