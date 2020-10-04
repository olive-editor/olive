#version 150

uniform sampler2D tex_in;
uniform float offset_in;
uniform float offset_r_in;
uniform float offset_g_in;
uniform float offset_b_in;
uniform float gamma_in;

uniform vec2 ove_resolution;
uniform int ove_iteration;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 texture_color = texture(tex_in, ove_texcoord);

    vec3 rgb = texture_color.rgb;

    // gamma
    rgb = pow(rgb, vec3(1/gamma_in));

    // offset
    rgb = vec3(rgb.r +  offset_r_in + offset_in, rgb.g +  offset_g_in + offset_in, rgb.b + offset_b_in + offset_in);

    fragColor =  vec4(rgb.r, rgb.g, rgb.b, 1.0);
}
