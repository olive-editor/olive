uniform float time_in;
varying vec2 ove_texcoord;

uniform float strength_in;
uniform bool color_in;
//uniform bool blend;

// precision lowp    float;

float PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
float PI  = 3.14159265358979323846264 *  00000.1; // PI
float SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two

bool isNan( float val )
{
  return ( val < 0.0 || 0.0 < val || val == 0.0 ) ? false : true;
  // important: some nVidias failed to cope with version below.
  // Probably wrong optimization.
  /*return ( val <= 0.0 || 0.0 <= val ) ? false : true;*/

  // Taken from: https://stackoverflow.com/questions/11810158/how-to-deal-with-nan-or-inf-in-opengl-es-2-0-shaders
}

float gold_noise(vec2 coordinate, float seed){
    float value = fract(tan(distance(coordinate*(seed+PHI), vec2(PHI, PI)))*SQ2)*(strength_in*0.01);
	return isNan(value) ? 0.0 : value;
}

void main(void) {
	vec3 noise;
	if (color_in) {
		noise = vec3(gold_noise(ove_texcoord, time_in + 42069.0), gold_noise(ove_texcoord, time_in + 69220.0), gold_noise(ove_texcoord, time_in + 1337.0));
	} else {
		noise = vec3(gold_noise(ove_texcoord, time_in + 69420.0));
	}

    gl_FragColor = vec4(noise, 1.0);
}
