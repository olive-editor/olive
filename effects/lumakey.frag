/* Luma key simple program
Based on Edward Cannon's Simple Chroma Key (adaptation by Olive Team)
Feel free to modify and use at will */

uniform sampler2D tex;
varying vec2 vTexCoord;

uniform float loc;
uniform float hic;
uniform bool invert;

void main(void) {
	vec4 texture_color = texture2D(tex,vTexCoord);

	float luma = max(max(texture_color.r,texture_color.g), texture_color.b) + min(min(texture_color.r,texture_color.g), texture_color.b);
	
	luma /= 2.0;

	if (luma > hic/100.0) {
        texture_color.a = (invert ? 0.0 : 1.0);
    } else if (luma < loc/100.0) {
        texture_color.a = (invert ? 1.0 : 0.0);
    } else {
        texture_color.a = (invert ? 1.0-luma : luma);
    }
	
	gl_FragColor = texture_color; 
}
