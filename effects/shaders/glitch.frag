#version 110



uniform float blockSize;

// uniform float block;
uniform float evolution;
// uniform bool colorShift;
uniform float colorShift;
uniform float negative;
uniform float darken;
// uniform float CSF;
// uniform float NF;
// uniform float DF;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

// float rand(vec2 co){
//     // return 1.0;
//     return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453) * 2.0 - 1.0;
// }

// float offset(float blocks, vec2 uv) {
//     float shaderTime = evolution*frequency;
//     // return rand(vec2(shaderTime, floor(uv.y * blocks)));
//     return rand(vec2(shaderTime, uv.y));
// }

float random(in float x){ return fract(sin(x)*43758.5453); }
float random(in vec2 st){ return fract(sin(dot(st.xy ,vec2(12.9898,78.233))) * 43758.5453); }

float OneTwoThree(in float x)
{
    float r = random(x);
    if (r > 0.66)
    {
        return 3.0;
    }
    else if (r > 0.33 )
    {
        return 2.0;
    }
    else
    {
        return 1.0;
    }
    
}


void main(void) {

    
    gl_FragColor = texture2D(myTexture, vTexCoord.xy);

    vec2 rblock = vec2(0.0,0.0);
    float e = floor(evolution*100.0) * 1.0;
    float v = 0.0;


    if (random(e*0.1) < colorShift)
    {
        rblock.x = random(e*0.2);
        rblock.y = random(e*0.3);

        v = 0.0;
        if (vTexCoord.x > rblock.x  && vTexCoord.x < (rblock.x + (blockSize * OneTwoThree(e*0.4))))
        {
            if (vTexCoord.y > rblock.y && vTexCoord.y < (rblock.y + blockSize))
            {
                v = 1.0 * (OneTwoThree(e*0.4));
            }
        }

        gl_FragColor.r = texture2D(myTexture, vTexCoord.xy + vec2(v * 0.3,0.0) ).r;
        gl_FragColor.g = texture2D(myTexture, vTexCoord.xy + vec2(v * 0.03 * 0.16666666, 0.0)).g;
        gl_FragColor.b = texture2D(myTexture, vTexCoord.xy + vec2(v * 0.03, 0.0)).b;
    }

    if (random(e*0.4) < negative)
    {

        rblock.x = random(e*0.6);
        rblock.y = random(e*0.7);

        // v = 0.0;
        if (vTexCoord.x > rblock.x  && vTexCoord.x < (rblock.x + (blockSize * OneTwoThree(e*0.8))))
        {
            if (vTexCoord.y > rblock.y && vTexCoord.y < (rblock.y + blockSize))
            {
                // v = 1.0 * (OneTwoThree(e+1.5));
                // gl_FragColor.r *= 50.0;
                gl_FragColor.r = 1.0 - gl_FragColor.r;
                gl_FragColor.g = 1.0 - gl_FragColor.g;
                gl_FragColor.b = 1.0 - gl_FragColor.b;
            }
        }

    }

    if (random(e*0.9) <darken)
    {

        rblock.x = random(e*0.1);
        rblock.y = random(e*0.2);

        // v = 0.0;
        if (vTexCoord.x > rblock.x  && vTexCoord.x < (rblock.x + (blockSize * OneTwoThree(e*0.3))))
        {
            if (vTexCoord.y > rblock.y && vTexCoord.y < (rblock.y + blockSize))
            {
                gl_FragColor = gl_FragColor*gl_FragColor*gl_FragColor*gl_FragColor*gl_FragColor;
            }
        }

    }






    // gl_FragColor.r *= r;

    // gl_FragColor.r =  texture2D(myTexture, vTexCoord.xy + offset).r;

    // gl_FragColor.r = texture2D(myTexture, vTexCoord.xy + vec2(x,0.0)).r;
    // gl_FragColor.r += r;

    // gl_FragColor.r = texture2D(myTexture, uv + offset(blockuv)).r;
    //gl_FragColor.r = texture2D(myTexture, uv + (vec2(offset(block) * 0.03), 0.0)).r;
    // gl_FragColor.g = texture2D(myTexture, uv + vec2(offset(block, uv) * 0.03 * 0.16666666, 0.0)).g;
    // gl_FragColor.b = texture2D(myTexture, uv + vec2(offset(block, uv) * 0.03, 0.0)).b;
    // gl_FragColor.a = 1.0;



	// float x = vTexCoord.x;
	// float y = vTexCoord.y;

	// if (vertical) {
	// 	x -= sin((vTexCoord.y-(evolution*0.01))*frequency)*intensity*0.01;
	// } else {
	// 	y -= sin((vTexCoord.x-(evolution*0.01))*frequency)*intensity*0.01;
	// }

	// if (y < 0.0 || y > 1.0 || x < 0.0 || x > 1.0) {
	// 	discard;
	// } else {
	// 	vec4 textureColor = texture2D(myTexture, vec2(x, y));
	// 	gl_FragColor = vec4(
	// 		textureColor.r,
	// 		textureColor.g,
	// 		textureColor.b,
	// 		textureColor.a
	// 	);
	// }	
}