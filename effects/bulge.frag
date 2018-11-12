#version 110

uniform float amount;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	float x = vTexCoord.x;
	float y = vTexCoord.y;

	float r = sqrt(pow(x - 0.5, 2.0) + pow(y - 0.5, 2.0));
	float a = atan(y - 0.5, x - 0.5);
	float rn = pow(r, (amount*0.1))/0.5;
	//float a = atan(x - 0.5, y - 0.5);

	x = rn * cos(a) + 0.5;
	y = rn * sin(a) + 0.5;

	if (y < 0.0 || y > 1.0 || x < 0.0 || x > 1.0) {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	} else {
		vec4 textureColor = texture2D(myTexture, vec2(x, y));
		gl_FragColor = vec4(
			textureColor.r,
			textureColor.g,
			textureColor.b,
			textureColor.a
		);
	}	
}