#version 150
uniform sampler2D tex0;
in vec2 vTexCoord;
out vec4 FragColor;

uniform float colors; // 12.0
uniform float sat_levels; // 4.0
uniform float bri_levels; // 4.0
uniform float edge_thres; // 0.2;
uniform float edge_thres2; // 5.0;

vec3 RGBtoHSV( float r, float g, float b) 
{
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

vec3 HSVtoRGB(float h, float s, float v ) 
{
	 int i;
	 float f, p, q, t;
	 vec3 res;

	 if( s == 0.0 ) 
	 {
			// achromatic (grey)
			res.x = v;
			res.y = v;
			res.z = v;
			return res;
	 }

	 h /= 60.0;         // sector 0 to 5
	 i = int(floor( h ));
	 f = h - float(i);         // factorial part of h
	 p = v * ( 1.0 - s );
	 q = v * ( 1.0 - s * f );
	 t = v * ( 1.0 - s * ( 1.0 - f ) );

	 switch(i) 
	 {
			case 0:
				 res.x = v;
				 res.y = t;
				 res.z = p;
				 break;
			case 1:
				 res.x = q;
				 res.y = v;
				 res.z = p;
				 break;
			case 2:
				 res.x = p;
				 res.y = v;
				 res.z = t;
				 break;
			case 3:
				 res.x = p;
				 res.y = q;
				 res.z = v;
				 break;
			case 4:
				 res.x = t;
				 res.y = p;
				 res.z = v;
				 break;
			default:      // case 5:
				 res.x = v;
				 res.y = p;
				 res.z = q;
				 break;
	 }
	 return res;
}

// averaged pixel intensity from 3 color channels
float avg_intensity(vec4 pix) 
{
 return (pix.r + pix.g + pix.b)/3.;
}

vec4 get_pixel(vec2 coords, float dx, float dy) 
{
 return texture(tex0,coords + vec2(dx, dy));
}

// returns pixel color
float IsEdge(in vec2 coords)
{
	float dxtex = 1.0 /float(textureSize(tex0,0)) ;
	float dytex = 1.0 /float(textureSize(tex0,0));
	float pix[9];
	int k = -1;
	float delta;

	// read neighboring pixel intensities
	for (int i=-1; i<2; i++) {
	 for(int j=-1; j<2; j++) {
		k++;
		pix[k] = avg_intensity(get_pixel(coords,float(i)*dxtex,
																					float(j)*dytex));
	 }
	}

	// average color differences around neighboring pixels
	delta = (abs(pix[1]-pix[7])+
					abs(pix[5]-pix[3]) +
					abs(pix[0]-pix[8])+
					abs(pix[2]-pix[6])
					 )/4.;

	//return clamp(5.5*delta,0.0,1.0);
	return clamp(edge_thres2*delta,0.0,1.0);
}

void main() {
	vec2 uv = vTexCoord.xy;
	vec4 tc = vec4(1.0, 0.0, 0.0, 1.0);
	vec3 colorOrg = texture(tex0, uv).rgb;
	vec3 vHSV =  RGBtoHSV(colorOrg.r,colorOrg.g,colorOrg.b);

	// hue limiting	
	if (colors == 0.0) {
		vHSV.y = 0.0;
	} else {
		float hue_divider = (360.0/colors);
		vHSV.x = round(vHSV.x/hue_divider)*hue_divider;

		float sat_divider = 100.0/sat_levels;
		vHSV.y = round((vHSV.y*100.0)/sat_divider)*sat_divider/100.0;
	}

	float bri_divider = 100.0/bri_levels;
	vHSV.z = round((vHSV.z*100.0)/bri_divider)*bri_divider/100.0;

	float edg = IsEdge(uv);
	vec3 vRGB = (edg >= (1.0-((edge_thres-1.0)*0.01)))? vec3(0.0,0.0,0.0):HSVtoRGB(vHSV.x,vHSV.y,vHSV.z);
	tc = vec4(vRGB.x,vRGB.y,vRGB.z, 1);
	FragColor = tc;
}