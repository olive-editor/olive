uniform sampler2D base_in;
uniform sampler2D blend_in;
uniform bool base_in_enabled;
uniform bool blend_in_enabled;

in vec2 ove_texcoord;
out vec4 frag_color;

void main(void) {
    vec4 base_col = texture(base_in, ove_texcoord);
    vec4 blend_col = texture(blend_in, ove_texcoord);

    if (!base_in_enabled && !blend_in_enabled) {
        frag_color = vec4(0.0);
        return;
    }

    if (!base_in_enabled) {
        frag_color = blend_col;
        return;
    }

    if (!blend_in_enabled) {
        frag_color = base_col;
        return;
    }

    base_col *= 1.0 - blend_col.a;
    base_col += blend_col;

    frag_color = base_col;
}
