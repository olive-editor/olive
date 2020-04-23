#version 150

// Inputs
uniform int orientation;
uniform vec3 color_a;
uniform vec3 color_b;

// Standard inputs
uniform vec2 ove_resolution;

in vec2 ove_texcoord;

out vec4 fragColor;

void main(void) {
    float t;

    // This very roughly conforms to Qt::Orientation
    if (orientation == 1) {
        // Horizontal orientation
        t = ove_texcoord.x;
    } else {
        // Vertical orientation
        t = 1.0 - ove_texcoord.y;
    }

    fragColor = vec4(mix(color_a, color_b, t), 1.0);
}
