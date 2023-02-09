uniform sampler2D tex_in;
uniform int color_in;
uniform int method_in;
uniform bool preserve_luminance_input;
uniform vec3 luma_coeffs;

in vec2 ove_texcoord;
out vec4 frag_color;

#define AVERAGE             0
#define DOUBLE_RED_AVERAGE  1
#define DOUBLE_AVERAGE      2
#define BLUE_LIMIT          3

void main(void) {
    vec4 original_col = texture(tex_in, ove_texcoord);
    vec4 tex_col = original_col;
    float color_average = 0.0;

    if(color_in == 0) { // Green screen
        switch (method_in) {
        case AVERAGE:
            color_average = dot(tex_col.rb, vec2(0.5)); // (tex_col.r + tex_col.b) / 2.0
            tex_col.g = tex_col.g > color_average ? color_average: tex_col.g;
            break;
        case DOUBLE_RED_AVERAGE:
            color_average = dot(tex_col.rb, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.r + tex_col.b) / 3.0
            tex_col.g = tex_col.g > color_average ? color_average : tex_col.g;
            break;
        case DOUBLE_AVERAGE:
            color_average = dot(tex_col.br, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.b + tex_col.r) / 3.0
            tex_col.g = tex_col.g > color_average ? color_average : tex_col.g;
            break;
        case BLUE_LIMIT:
            tex_col.g = tex_col.g > tex_col.b ? tex_col.b : tex_col.g;
            break;
        }
    } else { // Blue screen
        switch (method_in) {
        case AVERAGE:
            color_average = dot(tex_col.rg, vec2(0.5)); // (tex_col.r + tex_col.g) / 2.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
            break;
        case DOUBLE_RED_AVERAGE:
            color_average = dot(tex_col.rg, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.r + tex_col.g) / 3.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
            break;
        case DOUBLE_AVERAGE:
            color_average = dot(tex_col.gr, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.g+ tex_col.r) / 3.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
            break;
        case BLUE_LIMIT:
            tex_col.b = tex_col.b > tex_col.g ? tex_col.g : tex_col.b;
            break;
        }
    }

    if (preserve_luminance_input) {
        vec4 diff = original_col - tex_col;
        float luma = dot(abs(diff.rgb), luma_coeffs);
        tex_col.rgb += vec3(luma);
    }

    frag_color = tex_col;
}
