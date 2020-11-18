#version 150

uniform sampler2D ove_maintex;
uniform vec2 viewport;

uniform float histogram_scale;
uniform float histogram_power;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec3 col = vec3(0.0);
    float histogram_height = ceil(viewport.y * histogram_scale);
    vec3 histogram_ratio = vec3(0.0);
    vec3 sum = vec3(0.0);
    float ratio = 0.0;
    vec3 total_pixels = vec3(ceil(viewport.x * viewport.y *
        histogram_scale));

    for (int i = 0; i < histogram_height; i++) {
        ratio = float(i) / float(histogram_height - 1.0);
        sum += texture(
            ove_maintex,
            vec2(ove_texcoord.x, ratio)
        ).rgb;
    }

    histogram_ratio = pow(sum / total_pixels, vec3(histogram_power));
    col = step(vec3(ove_texcoord.y), histogram_ratio);

    fragColor = vec4(col, 1.0);
}
