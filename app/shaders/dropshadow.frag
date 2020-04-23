#version 150

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D tex_in;
uniform vec3 color_in;
uniform float softness_in;
uniform float opacity_in;
uniform float distance_in;
uniform float direction_in;

uniform vec2 ove_resolution;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    // Use pythagoras with the distance (hypotenuse) to find the shadow offset
    float direction_radians = direction_in * (M_PI / 180.0);

    float opposite = sin(direction_radians) * distance_in;
    float adjacent = cos(direction_radians) * distance_in;

    vec2 angle = vec2(adjacent, opposite);

    // Convert distance from pixels to 0.0 - 1.0 texture coordinates
    angle /= ove_resolution;

    float shadow_alpha;

    // For a soft shadow, we use a box blur-like formula
    if (softness_in > 0.0) {
        float radius = ceil(softness_in);
        float divider = 1.0 / pow(softness_in, 2.0);
        shadow_alpha = 0.0;

        for (float x = -radius + 0.5; x <= radius; x += 2.0) {
            for (float y = -radius + 0.5; y <= radius; y += 2.0) {
                vec2 pixel_coord = ove_texcoord - angle;
                pixel_coord.x += x / ove_resolution.x;
                pixel_coord.y += y / ove_resolution.y;
                vec4 pixel_color = texture(tex_in, pixel_coord);

                shadow_alpha += pixel_color.a * divider;
            }
        }
    } else {
        // Perfectly hard shadow
        vec4 src_color = texture(tex_in, ove_texcoord - angle);
        shadow_alpha = src_color.a;
    }

    vec4 shadow_px = vec4(color_in, shadow_alpha * opacity_in * 0.01);

    // Get current pixel and perform an alpha over for it over the shadow we've made
    vec4 dst_color = texture(tex_in, ove_texcoord);
    shadow_px *= (1.0 - dst_color.a);
    shadow_px += dst_color;

    fragColor = shadow_px;
}
