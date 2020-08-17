#version 150

uniform float waveform_scale;
uniform vec2 ove_resolution;

in vec4 a_position;
in vec2 a_texcoord;

out vec2 ove_texcoord;

mat4 scale_mat4(vec3 scale) {
    return mat4(
        scale.x, 0.0, 0.0, 0.0,
        0.0, scale.y, 0.0, 0.0,
        0.0, 0.0, scale.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main() {
    // Create identity matrix
    mat4 transform = mat4(1.0);

    // Scale the scope
    transform *= scale_mat4(vec3(waveform_scale, waveform_scale, 1.0));

    gl_Position = transform * a_position;
    ove_texcoord = a_texcoord;
}