#version 110

uniform float temperature;
uniform float tint;
uniform float exposure;
uniform float contrast;
uniform float highlights;
uniform float shadows;
uniform float whites;
uniform float blacks;
uniform float saturation;

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main(void) {
	vec4 textureColor = texture2D(myTexture, vTexCoord);

	vec3 rgb = textureColor.rgb;

	// temperature
	float temp = temperature * 0.01;
	float redTemp = (temp <= 66.0) ? 1.0 : min(1.0, max(0.0,
			(1.2929361861 * pow(temp - 60.0, -0.1332047592))
		));
	float greenTemp = min(1.0, max(0.0,
			(temp <= 66.0) ? (0.3900815788 * log(temp) - 0.6318414438) : (1.1298908609 * pow(temp - 60.0, -0.0755148492))
		));
	float blueTemp = (temp >= 66.0) ? 1.0 : min(1.0, max(0.0,
			(0.5432067891 * log(temp - 10.0) - 1.1962540891)
		));
	rgb *= vec3(redTemp, greenTemp, blueTemp);

	// tint
	rgb.g *= ((tint*0.01)+1.0);

	// exposure
	rgb *= pow(2.0, exposure*0.01);

	// contrast
	float contr_val = (contrast*0.01);
	rgb = (rgb*contr_val)-((contr_val-1.0)*0.5);


	// shadows/highlights
	//float luma = 0.33*rgb.r + 0.5*rgb.g + 0.16*rgb.b;
	if (rgb.r < 0.5 && rgb.g < 0.5 && rgb.b < 0.5) {
		float shadow_val = 1.0-(shadows*0.01);
		rgb = vec3(pow(rgb.r*2.0, shadow_val)*0.5, pow(rgb.g*2.0, shadow_val)*0.5, pow(rgb.b*2.0, shadow_val)*0.5);
	} else if (rgb.r > 0.5 && rgb.g > 0.5 && rgb.b > 0.5) {
		float highlight_val = 1.0-(highlights*0.01);
		rgb = vec3((pow((rgb.r-0.5)*2.0, highlight_val)*0.5)+0.5, (pow((rgb.g-0.5)*2.0, highlight_val)*0.5)+0.5, (pow((rgb.b-0.5)*2.0, highlight_val)*0.5)+0.5);
	}


	// whites
	rgb *= (whites*0.01);

	// blacks
	float black_val = 2.0-(blacks*0.01);
	rgb = (rgb*black_val)-(black_val-1.0);
	
	// saturation
	const vec3 W = vec3(0.2125, 0.7154, 0.0721);
	vec3 intensity = vec3(dot(rgb, W));
	rgb = mix(intensity, rgb, saturation*0.01);

	gl_FragColor = vec4(
		rgb.r,
		rgb.g,
		rgb.b,
		textureColor.a
	);
}
