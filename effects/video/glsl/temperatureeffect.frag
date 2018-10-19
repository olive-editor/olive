#version 110

// adapted from Tanner Helland's temperature algorithm
// (http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/)

uniform float temperature;

uniform mediump float amount_val;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vTexCoord);

	// red value
	float red = (temperature <= 66.0) ? 1.0 : min(1.0, max(0.0, (329.698727446 * (pow(temperature - 60.0, -0.1332047592)))/255.0));

	// green value
	float green = min(1.0, max(0.0, ((temperature <= 66.0) ? (99.4708025861 * log(temperature) - 161.1195681661) : 288.1221695283 * (pow(temperature - 60.0, -0.0755148492)))/255.0));

	// blue value
	float blue = (temperature >= 66.0) ? 1.0 : min(1.0, max(0.0, (138.5177312231 * log(temperature - 10.0) - 305.0447927307)/255.0));

	gl_FragColor = vec4(
		textureColor.r*red,
		textureColor.g*green,
		textureColor.b*blue,
		textureColor.a
	);
	/*gl_FragColor = vec4(
		red,
		green,
		blue,
		1.0
	);*/
}