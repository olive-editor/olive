uniform sampler2D tex_in;
uniform sampler2D garbage_in;
uniform sampler2D core_in;
uniform int color_in;
uniform bool garbage_in_enabled;
uniform bool core_in_enabled;
uniform float highlights_in;
uniform float shadows_in;
uniform bool mask_only_in;

in vec2 ove_texcoord;
out vec4 frag_color;

#define SCREEN_COLOR_GREEN 0
#define SCREEN_COLOR_BLUE  1

void main(void) {
    vec4 tex_col = texture(tex_in, ove_texcoord);

    // Unassociate RGB before calculating values
    vec4 unassoc = tex_col;
    if (unassoc.a > 0) {
      unassoc.rgb /= unassoc.a;
    }

    // Simple keyer, generates a inverted mask (background is white, foreground black)
    float mask;
    if (color_in == SCREEN_COLOR_GREEN) {
      mask = (unassoc.g - max(unassoc.r, unassoc.b));
    } else{ // Assume SCREEN_COLOR_BLUE
      mask = (unassoc.b - max(unassoc.r, unassoc.g));
    }

    mask = clamp(mask, 0.0, 1.0);

    if (garbage_in_enabled) {
      // Force anything we want to remove to be 1.0
      vec4 garbage = texture(garbage_in, ove_texcoord);
      // Assumes garbage is achromatic
      mask += garbage.r;
      mask = clamp(mask, 0.0, 1.0);
    }

    if (core_in_enabled) {
      // Force anything we want to keep to be 0.1
      vec3 core = texture(core_in, ove_texcoord).rgb;
      vec3 core_invert = 1.0 - core.rgb;
      // Assumes core is achromatic
      mask *= core_invert.r;
      mask = clamp(mask, 0.0, 1.0);
    }

    // Crush blacks and push whites
    mask = highlights_in * 0.01 * (shadows_in * 0.01 * mask - 1.0) + 1.0;
    mask = clamp(mask, 0.0, 1.0);

    // Invert mask
    mask = 1.0 - mask;

    // Multiply color by mask
    tex_col *= mask;

    if (!mask_only_in) {
        frag_color = tex_col;
    } else {
        frag_color = vec4(vec3(mask), 1.0);
    }
}
