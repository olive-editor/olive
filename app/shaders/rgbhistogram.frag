#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;

in vec2 ove_texcoord;
out vec4 fragColor;

void main(void) {
    float quantisation = 1.0 / ove_resolution.y;
    vec3 cur_col = vec3(0.0);
    vec3 sum = vec3(0.0);
    float ratio = 0.0;

    for (int i = 0; i < ove_viewport.x; i++) {
        ratio = float(i) / float(ove_viewport.x - 1);
        cur_col = texture(
            ove_maintex,
            vec2(ove_texcoord.y, ratio)
        ).rgb;

        sum += step(vec3(ove_texcoord.x - quantisation), cur_col) *
            step(cur_col, vec3(ove_texcoord.x + quantisation));
    }

    fragColor = vec4(sum, 1.0);
}
