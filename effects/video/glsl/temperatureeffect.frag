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
	float red = (temperature <= 66.0) ? 1.0 : min(1.0, max(0.0,
			(1.2929361861 * pow(temperature - 60.0, -0.1332047592))
		));

	// green value
	float green = min(1.0, max(0.0,
			(temperature <= 66.0) ? (0.3900815788 * log(temperature) - 0.6318414438) : (1.1298908609 * pow(temperature - 60.0, -0.0755148492))
		));

	// blue value
	float blue = (temperature >= 66.0) ? 1.0 : min(1.0, max(0.0,
			(0.5432067891 * log(temperature - 10.0) - 1.1962540891)
		));

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