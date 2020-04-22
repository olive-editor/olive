#version 150

uniform sampler2D base_in;
uniform sampler2D blend_in;
uniform bool base_in_enabled;
uniform bool blend_in_enabled;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 base_col = texture(base_in, ove_texcoord);
    vec4 blend_col = texture(blend_in, ove_texcoord);

    if (!base_in_enabled && !blend_in_enabled) {
        fragColor = vec4(0.0);
        return;
    }

    if (!base_in_enabled) {
        fragColor = blend_col;
        return;
    }

    if (!blend_in_enabled) {
        fragColor = base_col;
        return; 
    }

    base_col *= 1.0 - blend_col.a;
    base_col += blend_col;

    fragColor = base_col;
}
