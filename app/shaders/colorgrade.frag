#version 150

uniform sampler2D tex_in;
uniform vec4 offset_in;
uniform vec4 lift_in;
uniform vec4 gamma_in;
uniform vec4 gain_in;

uniform vec2 ove_resolution;
uniform int ove_iteration;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    vec4 texture_color = texture(tex_in, ove_texcoord);

    vec3 rgb = texture_color.rgb;

    // offset
    rgb = rgb +  offset_in.rgb;

    // lift
    rgb = lift_in.rgb + rgb * (vec3(1.0, 1.0, 1.0) - lift_in.rgb);

    // gamma
    rgb = pow(rgb, vec3(1.0 / gamma_in.r, 1.0 / gamma_in.g, 1.0 / gamma_in.b));

    // gain
    rgb = gain_in.rgb * rgb;


    fragColor =  vec4(rgb.r, rgb.g, rgb.b, 1.0);
}
