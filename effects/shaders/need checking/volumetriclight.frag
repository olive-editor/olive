uniform float amount;

uniform vec2 resolution; // screen resolution

vec4 process(vec4 col)  {
	float alpha = texture2D(texture, v_texcoord)[3];

	vec3 p = gl_FragCoord.xyz/vec3(resolution, 1.0)-.5;
	vec4 color = texture2D(texture,.5+(p.xy*=.992));
	vec3 o = color.rgb;
	for (float i=0.;i<amount;i++) {
		p.z += pow(max(0.,.5-length(color.rg)),2.)*exp(-i*.08);
	}
	return vec4(o*o+p.z, alpha);
}