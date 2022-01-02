uniform sampler2D tex_in;
uniform sampler2D garbage_in;
uniform sampler2D core_in;
uniform bool garbage_in_enabled;
uniform bool core_in_enabled;
uniform vec4 color_in;
uniform float min_level_in;
uniform float max_level_in;
uniform float contrast_in;
uniform bool mask_only_in;

varying vec2 ove_texcoord;


void main(void) {
    vec4 col_in = color_in;

    vec4 tex_col = texture2D(tex_in, ove_texcoord);
    
    // Simple keyer, generates a inverted mask (background is white, foreground black)
    float mask = (tex_col.g - max(tex_col.r, tex_col.b));
    mask = clamp(mask, 0.0, 1.0);


    if (garbage_in_enabled) {
      // Force anything we don't want to be 1.0
      vec4 garbage = texture2D(garbage_in, ove_texcoord);
      mask += garbage;
      mask = clamp(mask, 0.0, 1.0);
    }

    if (core_in_enabled) {
      // Force anything we want to be black
      vec4 core = texture2D(core_in, ove_texcoord);
      mask *= core;
      mask = clamp(mask, 0.0, 1.0);
    }

    // Push whites
    mask = contrast_in*mask;

    // Pull blacks
    //mask = contrast * (mask - 1) + 1

    // Combined
    // mask = blacks*(whites*x - 1) + 1


    if (mask < min_level_in) {
        mask = 0.0;
    }
    if (mask > max_level_in) {
        mask = 1.0;
    }

    mask = 1.0 - mask;

    tex_col.w = mask;

    tex_col.rgb *= tex_col.w;

    // Despill (should be another node really)
    tex_col.g = tex_col.g > (tex_col.r+tex_col.b)/2.0 ?(tex_col.r+tex_col.b)/2.0 : tex_col.g; 

    if (!mask_only_in) {
        gl_FragColor = tex_col;
    } else {
        gl_FragColor = vec4(mask, mask, mask, 1.0);
    }
}
