#version 150

uniform sampler2D ove_maintex;
uniform vec2 ove_resolution;
uniform vec2 ove_viewport;

uniform float histogram_scale;
uniform vec2 histogram_dims;
uniform vec4 histogram_region;
uniform vec4 histogram_uv;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    float quantisation = 1.0 / (histogram_dims.y - 1);

    // int y_lim = int(histogram_dims.y);

    vec3 col = vec3(0.0);
    vec3 sum = vec3(0.0);
    float total_pixels = ove_resolution.x * ove_resolution.y;
    vec3 histogram_ratio = vec3(0.0);
    vec3 cur_col = vec3(0.0);
    // vec3 cur_lum = vec3(0.0);

    if (
        (gl_FragCoord.x > histogram_region.x) &&
        (gl_FragCoord.y > histogram_region.y) &&
        (gl_FragCoord.x < histogram_region.z) &&
        (gl_FragCoord.y < histogram_region.w)
    ) {
        float ratio = 0.0;
        float histogram_x = (ove_texcoord.x - histogram_uv.x) /
            histogram_scale;
        float histogram_y = (ove_texcoord.y - histogram_uv.y) /
            histogram_scale;

        for (int i = 0; i < histogram_dims.y; i++) {
            ratio = float(i) / float(histogram_dims.y - 1);
            sum += texture(
                ove_maintex,
                vec2(histogram_x, ratio)
            ).rgb;
        }
        histogram_ratio = pow(sum / total_pixels, vec3(1.0 / 4.0));
        col = step(vec3(histogram_y), histogram_ratio);
    }

    fragColor = vec4(col, 1.0);
}
