#version 150

uniform sampler2D ove_maintex;
uniform vec2 viewport;
uniform float histogram_scale;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    float histogram_width = ceil(histogram_scale * viewport.y);
    float quantisation = 1.0 / (histogram_width - 1.0);
    vec3 cur_col = vec3(0.0);
    vec3 sum = vec3(0.0);
    float ratio = 0.0;

    for (int i = 0; i < histogram_width; i++) {
        ratio = float(i) / float(histogram_width - 1);
        cur_col = texture(
            ove_maintex,
            vec2(ove_texcoord.y, ratio)
        ).rgb;

        sum += step(vec3(ove_texcoord.x - quantisation), cur_col) *
            step(cur_col, vec3(ove_texcoord.x + quantisation)) +
            (
                // Account for values beyond the upper x limit.
                step(1.0 - quantisation, ove_texcoord.x) *
                step(vec3(1.0 - quantisation), cur_col)
            );
    }

    fragColor = vec4(sum, 1.0);
}
