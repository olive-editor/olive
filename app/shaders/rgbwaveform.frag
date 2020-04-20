// Adapted from "RGB Waveform" by lebek
// https://www.shadertoy.com/view/4dK3Wc

#version 110

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
varying vec2 ove_texcoord;

uniform float threshold;

void main(void) {
    vec3 col = vec3(0.0);
    float s = ove_texcoord.y*1.8 - 0.15;
    float maxb = s+threshold;
    float minb = s-threshold;
    
    int y_lim = int(ove_resolution.y);

    for (int i = 0; i < y_lim; i++) {
        vec3 x = texture2D(ove_maintex, vec2(ove_texcoord.x, float(i)/float(ove_resolution.y))).rgb;
        col += step(x, vec3(maxb))*step(vec3(minb), x) / (ove_resolution.y * 0.125);

        float l = dot(x, x);
        col += step(l, maxb*maxb)*step(minb*minb, l) / (ove_resolution.y * 0.125);
    }

    gl_FragColor = vec4(col, 1.0);
}
