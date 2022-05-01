uniform bool perspective_in;
uniform vec2 top_left_in;
uniform vec2 top_right_in;
uniform vec2 bottom_left_in;
uniform vec2 bottom_right_in;

uniform vec2 resolution_in;

uniform mat4 ove_mvpmat;

in vec4 a_position;
in vec2 a_texcoord;

out vec2 ove_texcoord;

out vec2 q;
out vec2 b1;
out vec2 b2;
out vec2 b3;

void main() {
    // The slider inputs only contain the amount they have changed rather than
    // their pixel locations so we adjust them here.
    vec2 t_l = top_left_in;
    vec2 t_r = top_right_in + vec2(resolution_in.x, 0.0);
    vec2 b_r = bottom_right_in + resolution_in;
    vec2 b_l = bottom_left_in + vec2(0.0, resolution_in.y);

    gl_Position = ove_mvpmat * a_position;

    if (perspective_in){
        // Find the center of the quadrilateral by finding where the two diagonals intersect.
        // https://www.reedbeta.com/blog/quadrilateral-interpolation-part-1/

        // Here we calculate the gradient and constant (y = mx + c) for each diagonal.
        float m1 = (t_r.y - b_l.y)/(t_r.x - b_l.x);
        float c1 = b_l.y - m1 * b_l.x;
        float m2 = (b_r.y - t_l.y)/(b_r.x - t_l.x);
        float c2 = t_l.y - m2 * t_l.x;

        // Find the intersection by setting the two line equations equal and rearrange.
        float mid_x = (c2 - c1) / (m1 - m2);
        float mid_y = m1 * mid_x + c1;

        // Find the distance from each corner to our center point
        float d0 = length(vec2(mid_x - b_l.x, mid_y - b_l.y));
        float d1 = length(vec2(b_r.x - mid_x, mid_y - b_r.y));
        float d2 = length(vec2(t_r.x - mid_x, t_r.y - mid_y));
        float d3 = length(vec2(mid_x - t_l.x, t_l.y - mid_y));

        float q  = 1.0;

        /*
        Vertex IDs (aspect ratio irrelevant):
             0_____1
            3|\    |
             | \   |
             |  \  |
             |   \ |
             |____\|2
             4     5
        */

        if (gl_VertexID == 0 || gl_VertexID == 3) {
            q = (d1+d3)/d3;
        } else if (gl_VertexID == 1) {
            q = (d0+d2)/d2;
        } else if (gl_VertexID == 2 || gl_VertexID == 5) {
            q = (d3+d1)/d1;
        } else {
            q = (d2+d0)/d0;
        }

        gl_Position[0] *= q;
        gl_Position[1] *= q;
        gl_Position[3] = q;
    } else{
        // https://www.reedbeta.com/blog/quadrilateral-interpolation-part-2/
        vec2 pos;

        if (gl_VertexID == 0 || gl_VertexID == 3) { // top left
            pos = t_l;
        } else if (gl_VertexID == 1) { // top right
            pos = t_r;
        } else if (gl_VertexID == 2 || gl_VertexID == 5) { // bottom right
            pos = b_r;
        } else if (gl_VertexID == 4) { // bottom left
            pos = b_l;
        }

        q = pos - b_l;
        b1 = b_r - b_l;
        b2 = t_l - b_l;
        b3 = b_l - b_r - t_l + t_r;
    }


    ove_texcoord = a_texcoord;
}
