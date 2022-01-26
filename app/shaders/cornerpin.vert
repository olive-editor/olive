#extension GL_EXT_gpu_shader4 : enable

uniform bool perspective;
uniform vec2 top_left_in;
uniform vec2 top_right_in;
uniform vec2 bottom_left_in;
uniform vec2 bottom_right_in;

uniform vec2 resolution_in;

uniform mat4 ove_mvpmat;

attribute vec4 a_position;
attribute vec2 a_texcoord;

varying vec2 ove_texcoord;

void main() {
    vec2 t_l = top_left_in;
    vec2 t_r = top_right_in + vec2(resolution_in.x, 0.0);
    vec2 b_r = bottom_right_in + resolution_in;
    vec2 b_l = bottom_left_in + vec2(0.0, resolution_in.y);

    gl_Position = ove_mvpmat * a_position;

    float m1 = (t_r.y - b_l.y)/(t_r.x - b_l.x);
    float c1 = b_l.y - m1 * b_l.x;
    float m2 = (b_r.y - t_l.y)/(b_r.x - t_l.x);
    float c2 = t_l.y - m2 * t_l.x;
    float mid_x = (c2 - c1) / (m1 - m2);
    float mid_y = m1 * mid_x + c1;

    float d0 = length(vec2(mid_x - b_l.x, mid_y - b_l.y));
    float d1 = length(vec2(b_r.x - mid_x, mid_y - b_r.y));
    float d2 = length(vec2(t_r.x - mid_x, t_r.y - mid_y));
    float d3 = length(vec2(mid_x - t_l.x, t_l.y - mid_y));



    /*
    float m1 = (bottom_right_in.y - top_left_in.y)/(bottom_right_in.x - top_left_in.x);
    float c1 = top_left_in.y - m1 * top_left_in.x;
    float m2 = (top_right_in.y - bottom_left_in.y)/(top_right_in.x - bottom_left_in.x);
    float c2 = bottom_left_in.y - m2 * bottom_left_in.x;
    float mid_x = (c2 - c1) / (m1 - m2);
    float mid_y = m1 * mid_x + c1;

    float d0 = length(vec2(mid_x - top_left_in.x, mid_y - top_left_in.y));
    float d1 = length(vec2(top_right_in.x - mid_x, mid_y - top_right_in.y));
    float d2 = length(vec2(bottom_right_in.x - mid_x, bottom_right_in.y - mid_y));
    float d3 = length(vec2(mid_x - bottom_left_in.x, bottom_left_in.y - mid_y));
    */


    /*
    float m1 = (p3.y - p0.y)/(p3.x - p0.x);
    float c1 = p0.y - m1 * p0.x;
    float m2 = (p1.y - p2.y)/(p1.x - p2.x);
    float c2 = p2.y - m2 * p2.x;
    float mid_x = (c2 - c1) / (m1 - m2);
    float mid_y = m1 * mid_x + c1;

    float d0 = length(vec2(mid_x - p0.x, mid_y - p0.y));
    float d1 = length(vec2(p1.x - mid_x, mid_y - p1.y));
    float d2 = length(vec2(p3.x - mid_x, p3.y - mid_y));
    float d3 = length(vec2(mid_x - p2.x, p2.y - mid_y));

    float q;

    if (gl_VertexID == 0) {
        q = (d1+d3)/d3;
    } else if (gl_VertexID == 1) {
        q = (d0+d2)/d2;
    } else if (gl_VertexID == 2) {
        q = (d3+d1)/d1;
    } else {
        q = (d2+d0)/d0;
    }

    */

    float q  = 1.0;

    if (gl_VertexID == 0) {
        q = (d1+d3)/d3;
        //q = d0/1920.0;
    } else if (gl_VertexID == 1) {
        q = (d0+d2)/d2;
       // q = 1.0;
    } else if (gl_VertexID == 2) {
        q = (d3+d1)/d1;
       // q = 1.0;
    } else {
        q = (d2+d0)/d0;
       // q = 2.0;
    }

    if(d0 == d1 && d1 == d2 && d2 == d3 && d3 == d0){
       // q = 2.0;
       // q = (d1+d3)/d3;
    }
    
    gl_Position[0] *= q;
    gl_Position[1] *= q;
    gl_Position[2] = 1.0;
    gl_Position[3] = q;

    

    ove_texcoord = a_texcoord;

    //vTexCoord = gl_MultiTexCoord0.xy;

}
