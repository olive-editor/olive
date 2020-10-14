#version 150

uniform sampler2D tex_in;
uniform vec4 slope_in;
uniform vec4 offset_in;
uniform vec4 power_in;

uniform vec2 ove_resolution;
uniform int ove_iteration;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 texture_color = texture(tex_in, ove_texcoord);

    vec4 color = texture_color;

    // slope
    color = color * slope_in;

    // offset
    color = color +  offset_in;

    // power
    color = pow(color, power_in);

    fragColor =  color;
}
