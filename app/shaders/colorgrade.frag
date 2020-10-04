#version 150

uniform sampler2D tex_in;
uniform float offset_in;
uniform float offset_r_in;
uniform float offset_g_in;
uniform float offset_b_in;

uniform vec2 ove_resolution;
uniform int ove_iteration;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 tex_color = texture(tex_in, ove_texcoord);
    fragColor =  vec4(tex_color.x +  offset_r_in + offset_in,
                      tex_color.y +  offset_g_in + offset_in,
                      tex_color.z +  offset_b_in + offset_in, 1.0);
}
