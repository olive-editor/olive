/* Original source code: https://www.youtube.com/watch?v=I23zT7iu_Y4
Adapted */

varying vec2 vTexCoord;
uniform vec2 resolution;
uniform sampler2D tex;

uniform float contrast100;
uniform float level;
uniform bool invert;
uniform bool hslmode; // luminance is 2*V

void main() {
    
    float contrast = contrast100 / 100.0;
    
	vec2 onePixel = vec2(floor(level) /  resolution.x, floor(level) /  resolution.y); // calculating the size of one pixel on the screen for the current resolution

	vec3 color = vec3(.5); // initialize color with half value on all channels
    if (invert) {
        color += texture2D(tex, vTexCoord - onePixel).rgb * contrast; 
	    color -= texture2D(tex, vTexCoord + onePixel).rgb * contrast;
    } else {
	    color -= texture2D(tex, vTexCoord - onePixel).rgb * contrast; 
	    color += texture2D(tex, vTexCoord + onePixel).rgb * contrast;
	}
	
	// original color
	vec4 color0 = texture2D(tex,vTexCoord);
	
	// grayscale
    float gray = (color.r + color.g + color.b) / (hslmode ? 3.0 : 1.5);
    
    //self-multiply
    color = color0.rgb * gray;
    
	gl_FragColor = vec4(color * color0.a, color0.a);
}
