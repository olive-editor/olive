uniform sampler2D tex;
varying vec2 vTexCoord;
uniform float time;
uniform vec2 resolution;

uniform float THRESHOLD;

uniform int mode;

#define PI 3.14159265
#define TILE_SIZE 16.0


float sat( float t ) {
	return clamp( t, 0.0, 1.0 );
}

vec2 sat( vec2 t ) {
	return clamp( t, 0.0, 1.0 );
}

//remaps inteval [a;b] to [0;1]
float remap  ( float t, float a, float b ) {
	return sat( (t - a) / (b - a) );
}

//note: /\ t=[0;0.5;1], y=[0;1;0]
float linterp( float t ) {
	return sat( 1.0 - abs( 2.0*t - 1.0 ) );
}


//note: [0;1]
float rand( vec2 n ) {
  return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

float rand(float n)
{
    return fract(sin(n) * 43758.5453123);
}

float noise(float p)
{
    float fl = floor(p);
  	float fc = fract(p);
    return mix(rand(fl), rand(fl + 1.0), fc);
}

float noise(vec2 p)
{
    vec2 ip = floor(p);
    vec2 u = fract(p);
    u = u * u * (3.0 - 2.0 * u);

    float res = mix(
        mix(rand(ip), rand(ip + vec2(1.0, 0.0)), u.x),
        mix(rand(ip + vec2(0.0,1.0)), rand(ip + vec2(1.0,1.0)), u.x), u.y);
    return res * res;
}


//note: [-1;1]
float srand( vec2 n ) {
	return rand(n) * 2.0 - 1.0;
}

float trunc( float x, float num_levels )
{
	return floor(x*num_levels) / num_levels;
}
vec2 trunc( vec2 x, float num_levels )
{
	return floor(x*num_levels) / num_levels;
}
vec2 trunc( vec2 x, vec2 num_levels )
{
	return floor(x*num_levels) / num_levels;
}



vec3 spectrum_offset( float t ) {
	vec3 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	float neg_w = 1.0-w;
	ret = vec3(lo,1.0,hi) * vec3(neg_w, w, neg_w);
	return ret;
}

vec3 rgb2yuv( vec3 rgb )
{
	vec3 yuv;
	yuv.x = dot( rgb, vec3(0.299,0.587,0.114) );
	yuv.y = dot( rgb, vec3(-0.14713, -0.28886, 0.436) );
	yuv.z = dot( rgb, vec3(0.615, -0.51499, -0.10001) );
	return yuv;
 }
 vec3 yuv2rgb( vec3 yuv )
 {
	vec3 rgb;
	rgb.r = yuv.x + yuv.z * 1.13983;
	rgb.g = yuv.x + dot( vec2(-0.39465, -0.58060), yuv.yz );
	rgb.b = yuv.x + yuv.y * 2.03211;
	return rgb;
 }

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 posterize(vec3 color, float steps)
{
    return floor(color * steps + 0.5) / steps;
}

float quantize(float n, float steps)
{
    return floor(n * steps) / steps;
}

vec4 downsample(sampler2D sampler, vec2 uv, float pixelSize)
{
    return texture2D(sampler, uv - mod(uv, vec2(pixelSize) / resolution.xy));
}

vec3 edge(sampler2D sampler, vec2 uv, float sampleSize)
{
    float dx = sampleSize / resolution.x;
    float dy = sampleSize / resolution.y;
    return (
    mix(downsample(sampler, uv - vec2(dx, 0.0), sampleSize), downsample(sampler, uv + vec2(dx, 0.0), sampleSize), mod(uv.x, dx) / dx) +
    mix(downsample(sampler, uv - vec2(0.0, dy), sampleSize), downsample(sampler, uv + vec2(0.0, dy), sampleSize), mod(uv.y, dy) / dy)    
    ).rgb / 2.0 - texture2D(sampler, uv).rgb;
}

vec3 distort(sampler2D sampler, vec2 uv, float edgeSize)
{
    vec2 pixel = vec2(1.0) / resolution.xy;
    vec3 field = rgb2hsv(edge(sampler, uv, edgeSize));
    vec2 distort = pixel * sin((field.rb) * PI * 2.0);
    float shiftx = noise(vec2(quantize(uv.y + 31.5, resolution.y / TILE_SIZE) * time, fract(time) * 300.0));
    float shifty = noise(vec2(quantize(uv.x + 11.5, resolution.x / TILE_SIZE) * time, fract(time) * 100.0));
    vec3 rgb = texture2D(sampler, uv + (distort + (pixel - pixel / 2.0) * vec2(shiftx, shifty) * (50.0 + 100.0 * (THRESHOLD*0.1))) * (THRESHOLD*0.1)).rgb;
    vec3 hsv = rgb2hsv(rgb);
    hsv.y = mod(hsv.y + shifty * pow((THRESHOLD*0.1), 5.0) * 0.25, 1.0);
    return posterize(hsv2rgb(hsv), floor(mix(256.0, pow(1.0 - hsv.z - 0.5, 2.0) * 64.0 * shiftx + 4.0, 1.0 - pow(1.0 - (THRESHOLD*0.1), 5.0))));
}



void main()
{
	if (mode==0)

	{

		float time_s = mod( time, 32.0 );

		float glitch_threshold = 1.0 - (THRESHOLD*0.1);
		const float max_ofs_siz = 0.1; //TOOD: input
		const float yuv_threshold = 0.5; //TODO: input, >1.0f == no distort
		const float time_frq = 16.0;

		vec2 uv = gl_FragCoord.xy / resolution.xy;
		
		const float min_change_frq = 4.0;
		float ct = trunc( time_s, min_change_frq );
		float change_rnd = rand( trunc(uv.yy,vec2(16)) + 150.0 * ct );

		float tf = time_frq*change_rnd;

		float t = 5.0 * trunc( time_s, tf );
		float vt_rnd = 0.5*rand( trunc(uv.yy + t, vec2(11)) );
		vt_rnd += 0.5 * rand(trunc(uv.yy + t, vec2(7)));
		vt_rnd = vt_rnd*2.0 - 1.0;
		vt_rnd = sign(vt_rnd) * sat( ( abs(vt_rnd) - glitch_threshold) / (1.0-glitch_threshold) );

		vec2 uv_nm = uv;
		uv_nm = sat( uv_nm + vec2(max_ofs_siz*vt_rnd, 0) );

		float rnd = rand( vec2( trunc( time_s, 8.0 )) );
		uv_nm.y = (rnd>mix(1.0, 0.975, sat((THRESHOLD*0.1)))) ? 1.0-uv_nm.y : uv_nm.y;

		vec4 sample = texture2D( tex, uv_nm, -10.0 );
		vec3 sample_yuv = rgb2yuv( sample.rgb );
		sample_yuv.y /= 1.0-3.0*abs(vt_rnd) * sat( yuv_threshold - vt_rnd );
		sample_yuv.z += 0.125 * vt_rnd * sat( vt_rnd - yuv_threshold );
		gl_FragColor = vec4( yuv2rgb(sample_yuv), sample.a );
	}

	else if (mode==1)

	{

		vec2 uv = gl_FragCoord.xy / resolution.xy;
		
		float my_time = mod(time, 32.0); // + modelmat[0].x + modelmat[0].z;

		float GLITCH = (THRESHOLD*0.1);
		
		float gnm = sat( GLITCH );
		float rnd0 = rand( trunc( vec2(my_time, my_time), 6.0 ) );
		float r0 = sat((1.0-gnm)*0.7 + rnd0);
		float rnd1 = rand( vec2(trunc( uv.x, 10.0*r0 ), my_time) ); //horz
		//float r1 = 1.0f - sat( (1.0f-gnm)*0.5f + rnd1 );
		float r1 = 0.5 - 0.5 * gnm + rnd1;
		r1 = 1.0 - max( 0.0, ((r1<1.0) ? r1 : 0.9999999) ); //note: weird ass bug on old drivers
		float rnd2 = rand( vec2(trunc( uv.y, 40.0*r1 ), my_time) ); //vert
		float r2 = sat( rnd2 );

		float rnd3 = rand( vec2(trunc( uv.y, 10.0*r0 ), my_time) );
		float r3 = (1.0-sat(rnd3+0.8)) - 0.1;

		float pxrnd = rand( uv + my_time );

		float ofs = 0.05 * r2 * GLITCH * ( rnd0 > 0.5 ? 1.0 : -1.0 );
		ofs += 0.5 * pxrnd * ofs;

		uv.y += 0.1 * r3 * GLITCH;

	    const int NUM_SAMPLES = 10;
	    const float RCP_NUM_SAMPLES_F = 1.0 / float(NUM_SAMPLES);
	    
		vec4 sum = vec4(0.0);
		vec3 wsum = vec3(0.0);
		for( int i=0; i<NUM_SAMPLES; ++i )
		{
			float t = float(i) * RCP_NUM_SAMPLES_F;
			uv.x = sat( uv.x + ofs * t );
			vec4 samplecol = texture2D( tex, uv, -10.0 );
			vec3 s = spectrum_offset( t );
			samplecol.rgb = samplecol.rgb * s;
			sum += samplecol;
			wsum += s;
		}
		sum.rgb /= wsum;
		sum.a *= RCP_NUM_SAMPLES_F;

		gl_FragColor.a = sum.a;
		gl_FragColor.rgb = sum.rgb; // * outcol0.a;
	}

	else if (mode==2)
	{
		vec2 uv = gl_FragCoord.xy / resolution.xy;
	    vec3 finalColor;
	    finalColor += distort(tex, uv, 8.0);
	    gl_FragColor = vec4(finalColor, 1.0);
	}


}
