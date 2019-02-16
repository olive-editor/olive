#version 130

uniform bool perspective;
uniform vec2 p0;
uniform vec2 p1;
uniform vec2 p2;
uniform vec2 p3;

varying vec2 q;
varying vec2 b1;
varying vec2 b2;
varying vec2 b3;
varying vec2 vTexCoord;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    if (perspective) {
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

        gl_Position[0] *= q;
        gl_Position[1] *= q;
        gl_Position[3] = q;

        vTexCoord = gl_MultiTexCoord0.xy;
    } else {
        vec2 pos;
        
        if (gl_VertexID == 0) { // top left
            pos = p2;
        } else if (gl_VertexID == 1) { // top right
            pos = p3;
        } else if (gl_VertexID == 2) { // bottom right
            pos = p1;
        } else if (gl_VertexID == 3) { // bottom left
            pos = p0;
        }

        q = pos - p0;
        b1 = p1 - p0;
        b2 = p2 - p0;
        b3 = p0 - p1 - p2 + p3;
    }
}