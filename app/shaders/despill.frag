uniform sampler2D tex_in;
uniform int color_in;
uniform int method_in;
uniform bool preserve_luminance_input;
uniform vec3 luma_coeffs;

varying vec2 ove_texcoord;

void main(void) {
    vec4 tex_col = texture2D(tex_in, ove_texcoord);
    float color_average = 0.0;

    if(color_in == 0) { // Green screen
        if(method_in == 0) { // Average
            color_average = dot(tex_col.rb, vec2(0.5)); // (tex_col.r + tex_col.b) / 2.0
            tex_col.g = tex_col.g > color_average ? color_average: tex_col.g; 
        } else if (method_in == 1) { // Double red average
            color_average = dot(tex_col.rb, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.r + tex_col.b) / 3.0
            tex_col.g = tex_col.g > color_average ? color_average : tex_col.g; 
        } else if (method_in == 2) { // Double average
            color_average = dot(tex_col.rb, vec2(0.5)); // (tex_col.r + tex_col.b) / 2.0
            tex_col.g = tex_col.g > 2.0 * color_average ? color_average: tex_col.g;
        } else if (method_in == 3) { // Blue Limit
            tex_col.g = tex_col.g > tex_col.b ? tex_col.b : tex_col.g;
        }
    } else { // Blue screen
        if(method_in == 0) { // Average
            color_average = dot(tex_col.rg, vec2(0.5)); // (tex_col.r + tex_col.g) / 2.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
        } else if (method_in == 1) { // Double red average
            color_average = dot(tex_col.rg, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.r + tex_col.g) / 3.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
        } else if (method_in == 2) { // Double average
            color_average = dot(tex_col.rb, vec2(0.5)); // (tex_col.r + tex_col.b) / 2.0
            tex_col.g = tex_col.g > 2.0 * color_average ? color_average: tex_col.g;
        } else if (method_in == 3) { // Blue Limit
            tex_col.b = tex_col.b > tex_col.g ? tex_col.g : tex_col.b; 
        }
    }

    if (preserve_luminance_input) {
        vec4 original_col = texture(iChannel0, uv);
        vec4 diff = original_col - tex_col;
        float luma = dot(abs(diff.rgb), luma_coeffs);
        tex_col.rgb += vec3(luma);
    }

    gl_FragColor = tex_col;
}
