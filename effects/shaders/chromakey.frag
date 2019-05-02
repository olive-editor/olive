/*a fast implementation of the chromakey program 
(c) 2008 Edward Cannon (adapted to GLSL by Olive Team)
feel free to use or modify at will*/
 
/*the following three functions convert RGB into YCbCr in the same manner as in JPEG images*/ 

uniform sampler2D tex;
varying vec2 vTexCoord;

uniform vec3 key_color;
uniform float tola;
uniform float tolb;
uniform bool opt;
uniform int mode;
 
float rgb2y (vec3 c) { 
	/*a utility function to convert colors in RGB into YCbCr*/ 

// This isn’t colour managed and is a huge mess, but in the
// short term, using correct weights will give significantly
// more ideal results. The correct weights for REC.709 are
// 0.2126 R, 0.7152 G, and 0.722 B. Easy change.
// The coefficients for YCbCr are calculated off of the
// REC.709 values.
	return (0.2126*c.r + 0.7152*c.g + 0.0722*c.b);
} 
 
float rgb2cb (vec3 c) { 
	/*a utility function to convert colors in RGB into YCbCr*/ 
	return (0.5 + -0.1145721061*c.r - 0.3854278939*c.g + 0.5*c.b);
} 
 
float rgb2cr (vec3 c) { 
	/*a utility function to convert colors in RGB into YCbCr*/ 
	return (0.5 + 0.5*c.r - 0.4541529083*c.g - 0.0458470917*c.b);
} 
 
float colorclose(float Cb_p,float Cr_p,float Cb_key,float Cr_key,float tola,float tolb) { 
	/*decides if a color is close to the specified hue*/ 
	float temp = sqrt(((Cb_key-Cb_p)*(Cb_key-Cb_p))+((Cr_key-Cr_p)*(Cr_key-Cr_p)));
	if (temp < tola) {return (0.0);} 
	if (temp < tolb) {return ((temp-tola)/(tolb-tola));} 
	return (1.0); 
}

void main(void) {
	float cb_key = rgb2cb(key_color);
	float cr_key = rgb2cr(key_color);
	
	vec4 texture_color = texture2D(tex, vTexCoord);

	float cb = rgb2cb(texture_color.rgb); 
	float cr = rgb2cr(texture_color.rgb); 
	float mask = colorclose(cb, cr, cb_key, cr_key, (tola/100.0), (tolb/100.0));

	if (mode == 0) { // composite
		//float submask = 1.0-mask;
		float submask = 0.0;
		texture_color.r = max(texture_color.r - submask*key_color.r, 0.0) + submask;
		texture_color.g = max(texture_color.g - submask*key_color.g, 0.0) + submask;
		texture_color.b = max(texture_color.b - submask*key_color.b, 0.0) + submask;
		texture_color.a *= mask;

		// premultiply
		texture_color.rgb *= texture_color.a;
	} else if (mode == 1) { // alpha
		texture_color.rgb = vec3(mask);
	} else if (mode == 2) { // original
	}
	
	gl_FragColor = texture_color;
}
