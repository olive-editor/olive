uniform sampler2D tex_in;
uniform int method_in;
uniform int pixels_in;
uniform vec2 resolution_in;

uniform int ove_iteration;

in vec2 ove_texcoord;
out vec4 frag_color;

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

    pixel_coord = ove_texcoord;

    // Constants for gaussian method
    float sigma = float(size) / 2.0;
    float max_weight = gaussian2(0.0, 0.0, sigma);
    size*=3;

    // GLSL has no FLOAT_MIN or FLOAT_MAX
    // Gaussian erode essentially inverts the image, does a gaussian dilate and then
    // reinverts the image hence there being a special case here for method_in == 2
    composite = pixels_in > 0 || method_in == 2 ? vec4(-9999.0) : vec4(9999.0);
    for(int i = -size; i <= size; i++) {

        if(ove_iteration == 0){
            offset.x = float(i) / resolution_in.x;
            offset.y = 0.0;
        } else{
            offset.x = 0.0;
            offset.y = float(i) / resolution_in.y;
        }

        if (method_in == 0) { // Box
            sample = texture(tex_in, pixel_coord+offset);
            if (pixels_in > 0) {
                composite = max(sample, composite);
            } else if (pixels_in < 0) {
                composite = min(sample, composite);
            }
        } else if (method_in == 1) { // Distance
            float len = length(offset);
            float scaled_size = float(size) / length(resolution_in);
            if (len <= scaled_size){
                sample = texture(tex_in, pixel_coord+offset);
                if (pixels_in > 0) {
                    composite = max(sample, composite);
                } else if (pixels_in < 0) {
                    composite = min(sample, composite);
                }
            }
        } else if (method_in == 2) { // Gaussian
            float weight = gaussian2(float(i), 0.0, sigma) / max_weight;
            sample = texture(tex_in, pixel_coord+offset);
            if (pixels_in > 0) {
                // weight^2 seems to give a better result
                composite = max(sample*weight*weight, composite);
            } else if (pixels_in < 0) {
                composite = max((1.0-sample)*weight*weight, composite);
            }
        }
    }
    if (method_in == 2 && pixels_in < 0) {
        frag_color = vec4(1.0-composite);
    } else{
        frag_color = vec4(composite);
    }
}
