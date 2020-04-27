// Adapted from "RGB Waveform" by lebek
// https://www.shadertoy.com/view/4dK3Wc

#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;
varying vec2 ove_texcoord;


uniform float threshold;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec3 col = vec3(0.0);
    float increment = 1.0 / 255.0;
    float maxb = ove_texcoord.y + increment;
    float minb = ove_texcoord.y - increment;

    int y_lim = int(ove_resolution.y);

    vec3 cur_col = vec3(0.0);
    for (int i = 0; i < y_lim; i++) {
        cur_col = texture2D(
            ove_maintex,
            vec2(ove_texcoord.x, float(i) / float(ove_resolution.y))
        ).rgb;

        col += step(vec3(ove_texcoord.y - increment), cur_col) *
            step(cur_col, vec3(ove_texcoord.y + increment));

        //float l = dot(x, x);
        //col += step(l, maxb*maxb)*step(minb*minb, l) / (ove_resolution.y * 0.125);
    }

    gl_FragColor = vec4(col, 1.0);
}
