#version 110

uniform vec2 resolution;
uniform float time;

uniform float amount;
uniform bool color;
uniform bool blend;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

// precision lowp    float;

float PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
float PI  = 3.14159265358979323846264 * 00000.1; // PI
float SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two

float gold_noise(vec2 coordinate, float seed){
    return fract(tan(distance(coordinate*(seed+PHI), vec2(PHI, PI)))*SQ2)*(amount*0.01);
}

void main(void) {
	vec3 noise;
	if (color) {
		noise = vec3(gold_noise(vTexCoord, time + 42069.0), gold_noise(vTexCoord, time + 69220.0), gold_noise(vTexCoord, time + 1337.0));
	} else {
		noise = vec3(gold_noise(vTexCoord, time + 69420.0));
	}

	if (blend) {
		noise = (noise - vec3(amount*0.005))*vec3(2.0);

		vec4 textureColor = texture2D(myTexture, vec2(vTexCoord.x, vTexCoord.y));
		gl_FragColor = vec4(textureColor.rgb+noise, textureColor.a);
	} else {
		gl_FragColor = vec4(noise, 1.0);
	}
}

/*void main(void) {
	vec4 textureColor = texture2D(myTexture, vec2(vTexCoord.x, vTexCoord.y));
	textureColor.r += gold_noise(resolution);
	textureColor.g += gold_noise(resolution);
	textureColor.b += gold_noise(resolution);
	gl_FragColor = textureColor;
}*/