uniform sampler2D tex_in;
uniform int pixels_in;
uniform vec2 resolution_in;

uniform int ove_iteration;

varying vec2 ove_texcoord;

void main(void) {
    vec2 pixel_coord;
    vec4 sample;
    vec3 composite;

    if (pixels_in == 0) {
        gl_FragColor = texture2D(tex_in, ove_texcoord);
        return;
    }
    
    if (pixels_in > 0) {
        composite = vec3(-9999.0); // GLSL has no FLOAT_MIN
        for (int i = -pixels_in; i < pixels_in; i++) {
            pixel_coord = ove_texcoord;
            if (ove_iteration == 0) {
                pixel_coord.x += float(i) / resolution_in.x;
            } else if (ove_iteration == 1) {
                pixel_coord.y += float(i) / resolution_in.y;
            }
            sample = texture2D(tex_in, pixel_coord);
            composite = max(sample.rgb, composite);
        }
    } else if (pixels_in < 0) {
        composite = vec3(9999.0); // GLSL has no FLOAT_MAX
        for (int i = pixels_in; i < -pixels_in; i++) {
            pixel_coord = ove_texcoord;
            if (ove_iteration == 0) {
                pixel_coord.x += float(i) / resolution_in.x;
            } else if (ove_iteration == 1) {
                pixel_coord.y += float(i) / resolution_in.y;
            }
            sample = texture2D(tex_in, pixel_coord);
            composite = min(sample.rgb, composite);
        }
    }

    gl_FragColor = vec4(composite, 1.0);
}