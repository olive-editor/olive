uniform sampler2D ove_maintex;

uniform vec2 viewport;
uniform vec3 luma_coeffs;

uniform float waveform_scale;

varying vec2 ove_texcoord;

void main(void) {
    float waveform_height = ceil(waveform_scale * viewport.y);
    float quantisation = 1.0 / (waveform_height - 1.0);
    float intensity = 0.10;
    vec4 col = vec4(0.0);
    vec4 cur_col = vec4(0.0);
    float ratio = 0.0;

    for (int i = 0; float(i) < waveform_height; i++) {
        ratio = float(i) / float(waveform_height - 1.0);
        cur_col.rgb = texture2D(
            ove_maintex,
            vec2(ove_texcoord.x, ratio)
        ).rgb;

        cur_col.w = dot(cur_col.rgb, luma_coeffs);

        col += (
            step(vec4(ove_texcoord.y - quantisation), cur_col) *
            step(cur_col, vec4(ove_texcoord.y + quantisation)) *
            intensity) +
            (step(1.0 - quantisation, ove_texcoord.y) *
            step(vec4(1.0 - quantisation), cur_col) * intensity);
    }

    col.rgb += vec3(col.w);
    gl_FragColor = vec4(col.rgb, 1.0);
}
