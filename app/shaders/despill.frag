uniform sampler2D tex_in;
uniform int color_in;
uniform int method_in;
uniform bool preserve_luminance_input;
uniform vec3 luma_coeffs;

varying vec2 ove_texcoord;

void main(void) {
    vec4 tex_col = texture2D(tex_in, ove_texcoord);

    if(color_in == 0) { // Green screen
        if (method_in == 0) { // Average
            tex_col.g = tex_col.g > (tex_col.r+tex_col.b)/2.0 ? (tex_col.r+tex_col.b)/2.0 : tex_col.g; 
        }
        if (method_in == 1) { // Double red average
            tex_col.g = tex_col.g > (2.0*tex_col.r+tex_col.b)/3.0 ? (2.0*tex_col.r+tex_col.b)/3.0 : tex_col.g; 
        }
        if (method_in == 2) { // Double average
            tex_col.g = tex_col.g > (2.0*tex_col.b+tex_col.r)/3.0 ? (2.0*tex_col.b+tex_col.r)/3.0 : tex_col.g; 
        }
        if (method_in == 3) { // Blue Limit
            tex_col.g = tex_col.g > tex_col.b ? tex_col.b : tex_col.g; 
        }
    } else { // Blue screen
        if (method_in == 0) { // Average
            tex_col.b = tex_col.b > (tex_col.r+tex_col.g)/2.0 ? (tex_col.r+tex_col.g)/2.0 : tex_col.b; 
        }
        if (method_in == 1) { // Double red average
            tex_col.b = tex_col.b > (2.0*tex_col.r+tex_col.g)/3.0 ? (2.0*tex_col.r+tex_col.g)/3.0 : tex_col.b; 
        }
        if (method_in == 2) { // Double average
            tex_col.b = tex_col.b > (2.0*tex_col.g+tex_col.r)/3.0 ? (2.0*tex_col.g+tex_col.r)/3.0 : tex_col.b; 
        }
        if (method_in == 3) { // Green Limit
            tex_col.b = tex_col.b > tex_col.g ? tex_col.g : tex_col.b; 
        }

    }

    if (preserve_luminance_input) {
        vec4 original_col = texture2D(tex_in, ove_texcoord);
        vec4 diff = abs(original_col - tex_col);
        float luma = dot(diff.rgb, luma_coeffs);
        tex_col.rgb += vec3(luma, luma, luma);
    }



    gl_FragColor = tex_col;
}