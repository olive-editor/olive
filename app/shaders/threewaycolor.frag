uniform sampler2D tex_in;

uniform vec4 shadows_color_in;
uniform vec4 midtones_color_in;
uniform vec4 highlights_color_in;
uniform float shadows_amount_in;
uniform float midtones_amount_in;
uniform float highlights_amount_in;
uniform vec3 luma_coefficients_in;

in vec2 ove_texcoord;
out vec4 frag_color;

vec3 color_offset(vec4 control, float amount)
{
    return (control.rgb - vec3(0.5)) * 2.0 * amount;
}

void main(void)
{
    vec4 source = texture(tex_in, ove_texcoord);
    float luma = clamp(dot(source.rgb, luma_coefficients_in), 0.0, 1.0);

    float shadow_weight = smoothstep(0.75, 0.0, luma);
    float highlight_weight = smoothstep(0.25, 1.0, luma);
    float midtone_weight = clamp(1.0 - abs(luma - 0.5) * 2.0, 0.0, 1.0);

    vec3 adjustment =
        color_offset(shadows_color_in, shadows_amount_in) * shadow_weight +
        color_offset(midtones_color_in, midtones_amount_in) * midtone_weight +
        color_offset(highlights_color_in, highlights_amount_in) * highlight_weight;

    vec3 graded = source.rgb + adjustment * source.rgb * (1.0 - source.rgb);
    frag_color = vec4(clamp(graded, 0.0, 1.0), source.a);
}
