#version 150

uniform sampler2D tex_in;
uniform vec3 slope_in;
uniform vec3 offset_in;
uniform vec3 power_in;

uniform vec2 ove_resolution;
uniform int ove_iteration;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 texture_color = texture(tex_in, ove_texcoord);

    vec3 rgb = texture_color.rgb;

    // slope
    rgb = rgb * slope_in;

    // offset
    rgb = rgb +  offset_in;

    // power
    rgb = pow(rgb, power_in);

    fragColor =  vec4(rgb.r, rgb.g, rgb.b, 1.0);
}
