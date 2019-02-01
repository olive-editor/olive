/* Filter by color characteristic simple program
Based on Edward Cannon's Simple Chroma Key (adaptation by Olive Team)
RGB to HSV based on MattKC's toonify source code
Feel free to modify and use at will */
#version 110

uniform sampler2D tex;
varying vec2 vTexCoord;

uniform float loc;
uniform float hic;
uniform int compo;
uniform bool invert;

float rgb2luma(vec3 c) {
    return (max(max(c.r,c.g), c.b) + min(min(c.r,c.g), c.b))/2.0;
}

vec3 rgb2hsv(vec3 c) 
{
     float r = c.r;
     float b = c.b;
     float g = c.g;
	 float minv, maxv, delta;
	 vec3 res;

	 minv = min(min(r, g), b);
	 maxv = max(max(r, g), b);
	 res.z = maxv;            // v
	 
	 delta = maxv - minv;

	 if( maxv != 0.0 )
			res.y = delta / maxv;      // s
	 else {
			// r = g = b = 0      // s = 0, v is undefined
			res.y = 0.0;
			res.x = -1.0;
			return res;
	 }

	 if( r == maxv )
			res.x = ( g - b ) / delta;      // between yellow & magenta
	 else if( g == maxv )
			res.x = 2.0 + ( b - r ) / delta;   // between cyan & yellow
	 else
			res.x = 4.0 + ( r - g ) / delta;   // between magenta & cyan

	 res.x = res.x * 60.0;            // degrees
	 if( res.x < 0.0 )
			res.x = res.x + 360.0;
			
	 return res;
}

bool isNotIncreasingSequence(float a, float b, float c) {
    return (c < b || a > b);
}

void main(void) {
	vec4 texture_color = texture2D(tex,vTexCoord);
    vec3 color = texture_color.rgb;
    float toCheck = 0.0;
    
	if (compo == 0) {
            toCheck = rgb2luma(color)*100.0;
    } else if (compo == 4) {
            toCheck = color.r*100.0;
    } else if (compo == 5) {
            toCheck = color.g*100.0;
    } else if (compo == 6) {
            toCheck = color.b*100.0;
    } else if (compo == 1) {
            toCheck = rgb2hsv(color).z*100.0;
    } else if (compo == 2) {
            toCheck = rgb2hsv(color).x/3.6;
    } else if (compo == 3) {
            toCheck = rgb2hsv(color).y*100.0;
    }
	texture_color.a = isNotIncreasingSequence(loc, toCheck, hic) ? (invert ? texture_color.a : 0.0) : (invert ? 0.0 : texture_color.a);
	texture_color.rgb *= texture_color.a;
	gl_FragColor = texture_color;
}
