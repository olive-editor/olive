uniform sampler2D tex_in;
//uniform sampler2D garbage_in;
uniform vec4 color_in;
uniform float min_level_in;
uniform float max_level_in;
uniform float contrast_in;
uniform bool mask_only_in;

varying vec2 ove_texcoord;


void main(void) {
    vec4 col_in = color_in;

    vec4 tex_col = texture2D(tex_in, ove_texcoord);

    //vec4 garbage = texture2D(garbage_in, ove_texcoord);

    //float temp = sqrt( (tex_col.r - col_in.r)*(tex_col.r - col_in.r)+(tex_col.g - col_in.g)*(tex_col.g - col_in.g)+(tex_col.b - col_in.b)*(tex_col.b - col_in.b));
    
    float temp = (tex_col.g - max(tex_col.r, tex_col.b));

    //temp *= garbage;

    temp = contrast_in*temp;;

    if (temp < min_level_in) {
        temp = 0.0;
    }
    if (temp > max_level_in) {
        temp = 1.0;
    }

    temp = 1.0 - temp;

    tex_col.w = temp;

    tex_col.rgb *= tex_col.w;

    // Despill (should be another node really)
    tex_col.g = tex_col.g > (tex_col.r+tex_col.b)/2.0 ?(tex_col.r+tex_col.b)/2.0 : tex_col.g; 

    if (!mask_only_in) {
        gl_FragColor = tex_col;
    } else {
        gl_FragColor = vec4(temp, temp, temp, 1.0);
    }
}
