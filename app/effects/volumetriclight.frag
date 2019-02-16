#version 110

uniform float amount;

uniform sampler2D tex0;
uniform vec2 resolution; // screen resolution
varying vec2 vTexCoord;
#define T texture2D(tex0,.5+(p.xy*=.992))

void main()  {
	float alpha = texture2D(tex0, vTexCoord)[3];

	vec3 p = gl_FragCoord.xyz/vec3(resolution, 1.0)-.5;
	vec3 o = T.rgb;
	for (float i=0.;i<amount;i++) {
		p.z += pow(max(0.,.5-length(T.rg)),2.)*exp(-i*.08);
	}
	gl_FragColor=vec4(o*o+p.z, alpha);
}