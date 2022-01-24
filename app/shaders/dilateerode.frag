uniform sampler2D tex_in;
uniform int method_in;
uniform int pixels_in;
uniform vec2 resolution_in;

uniform int ove_iteration;

varying vec2 ove_texcoord;

// Gaussian function uses PI
#define M_PI 3.1415926535897932384626433832795

float gaussian2(float x, float y, float sigma) {
    return (1.0/((sigma*sigma)*2.0*M_PI))*exp(-0.5*(((x*x) + (y*y))/(sigma*sigma)));
}

void main(void) {
    vec2 pixel_coord;
    vec2 offset;
    vec4 sample;
    vec4 composite;

    int size = int(abs(float(pixels_in)));

    if (pixels_in == 0) {
        gl_FragColor = texture2D(tex_in, ove_texcoord);
        return;
    }
    

    pixel_coord = ove_texcoord;
    float max_weight = gaussian2(0.0, 0.0, float(size)/2.0);
    size*=3;

    // GLSL has no FLOAT_MIN or FLOAT_MAX
    composite = pixels_in > 0? vec4(-9999.0) : vec4(9999.0);
    for (int j = -size; j <= size; j++) {
        for(int i = -size; i <= size; i++) {
            offset.x = float(i) / resolution_in.x;
            offset.y = float(j) / resolution_in.y;
            if (method_in == 0) {
                sample = texture2D(tex_in, pixel_coord+offset);
                if (pixels_in > 0) {
                    composite = max(sample, composite);
                } else if (pixels_in < 0) {
                    composite = min(sample, composite);
                }
            } else if (method_in == 1) {
                float len = length(offset);
                float scaled_size = float(size) / length(resolution_in);
                if (len <= scaled_size){
                    sample = texture2D(tex_in, pixel_coord+offset);
                    if (pixels_in > 0) {
                        composite = max(sample, composite);
                    } else if (pixels_in < 0) {
                        composite = min(sample, composite);
                    } 
                }
            } else if (method_in == 2) {
                float weight = gaussian2(float(i), float(j), float(size)/6.0) / max_weight;

                sample = texture2D(tex_in, pixel_coord+offset);
                if (pixels_in > 0) {
                    composite = max(sample*weight*weight, composite);
                } else if (pixels_in < 0) {
                    composite = min(sample, composite);
                }
            }
        }
    }

    gl_FragColor = vec4(composite);
}
