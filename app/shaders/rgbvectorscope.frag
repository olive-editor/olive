uniform sampler2D ove_maintex;

uniform vec2 viewport;
uniform vec3 luma_coeffs;

uniform float vectorscope_gain;
uniform float vectorscope_point_radius;
uniform float vectorscope_intensity;
uniform float vectorscope_sample_grid;

in vec2 ove_texcoord;
out vec4 frag_color;

vec2 rgb_to_vectorscope(vec3 rgb)
{
    float y = dot(rgb, luma_coeffs);
    float cb = (rgb.b - y) / max(2.0 * (1.0 - luma_coeffs.b), 0.0001);
    float cr = (rgb.r - y) / max(2.0 * (1.0 - luma_coeffs.r), 0.0001);
    return vec2(cr, cb) * vectorscope_gain + vec2(0.5);
}

void main(void)
{
    float grid = max(vectorscope_sample_grid, 1.0);
    float point_radius = vectorscope_point_radius / max(min(viewport.x, viewport.y), 1.0);
    vec3 accumulated = vec3(0.0);

    for (int y = 0; y < 64; y++) {
        if (float(y) >= grid) {
            break;
        }

        for (int x = 0; x < 64; x++) {
            if (float(x) >= grid) {
                break;
            }

            vec2 sample_uv = (vec2(float(x), float(y)) + vec2(0.5)) / grid;
            vec3 sample_rgb = clamp(texture(ove_maintex, sample_uv).rgb, 0.0, 1.0);
            vec2 scope_point = rgb_to_vectorscope(sample_rgb);
            float distance_to_point = distance(ove_texcoord, scope_point);
            float contribution = 1.0 - smoothstep(point_radius, point_radius * 2.0, distance_to_point);
            accumulated += sample_rgb * contribution * vectorscope_intensity;
        }
    }

    frag_color = vec4(clamp(accumulated, 0.0, 1.0), 1.0);
}
