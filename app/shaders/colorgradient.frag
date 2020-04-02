#version 110

// Standard inputs
uniform vec2 ove_resolution;
varying vec2 ove_texcoord;

// Inputs
uniform int orientation;
uniform vec3 color_a;
uniform vec3 color_b;

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

    gl_FragColor = vec4(mix(color_a, color_b, t), 1.0);
}
