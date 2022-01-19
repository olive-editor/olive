uniform sampler2D tex_in;
uniform sampler2D garbage_in;
uniform sampler2D core_in;
uniform int color_in;
uniform bool garbage_in_enabled;
uniform bool core_in_enabled;
uniform float darks_in;
uniform float brights_in;
uniform float contrast_in;
uniform bool mask_only_in;

varying vec2 ove_texcoord;

void main(void) {
    vec4 tex_col = texture2D(tex_in, ove_texcoord);
    
    // Simple keyer, generates a inverted mask (background is white, foreground black)
    float mask;
    if (color_in == 0) {
      mask = (tex_col.g - max(tex_col.r, tex_col.b));
    } else{
      mask = (tex_col.b - max(tex_col.r, tex_col.g));
    }

    mask = clamp(mask, 0.0, 1.0);

    if (garbage_in_enabled) {
      // Force anything we want to remove to be 1.0
      vec4 garbage = texture2D(garbage_in, ove_texcoord);
      // Assumes garbage is achromatic
      mask += garbage.r;
      mask = clamp(mask, 0.0, 1.0);
    }

    if (core_in_enabled) {
      // Force anything we want to keep to be 0.1
      vec3 core = texture2D(core_in, ove_texcoord).rgb;
      vec3 core_invert = 1.0 - core.rgb;
      // Assumes core is achromatic
      mask *= core_invert.r;
      mask = clamp(mask, 0.0, 1.0);
    }

    // Crush blacks and push whites
    mask = darks_in * (brights_in * mask - 1.0) + 1.0;
    mask = clamp(mask, 0.0, 1.0);

    // Invert mask
    mask = 1.0 - mask;

    // Set Alpha channel as mask
    tex_col.w = mask;

    // Pre mulitply
    tex_col.rgb *= tex_col.w;

    if (!mask_only_in) {
        gl_FragColor = tex_col;
    } else {
        gl_FragColor = vec4(vec3(mask), 1.0);
    }
}
