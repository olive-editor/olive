#version 110

uniform sampler2D image;

uniform vec2 resolution;

uniform int shadow;
uniform vec3 shadowcolor;
uniform float shadowsoftness;
uniform float shadowopacity;
uniform float shadowdistance;

varying vec2 vTexCoord;

void main(void) {
	vec4 master_px = texture2D(image, vTexCoord);
	if (shadow == 1) {
		vec2 shadow_dist = vec2(shadowdistance)/resolution;

		float shadow_opacity = 0.01*shadowopacity;

		vec4 shadowed_px = vec4(shadowcolor.r, shadowcolor.g, shadowcolor.b, shadow_opacity*texture2D(image, vTexCoord-shadow_dist).a);

		float rad = floor(shadowsoftness);
		if (rad > 0.0) {
			shadowed_px.a = 0.0;
			float divider = (1.0 / pow(rad, 2.0))*shadow_opacity;
			for (float x=-rad+0.5;x<=rad;x+=2.0) {					
				for (float y=-rad+0.5;y<=rad;y+=2.0) {
					shadowed_px.a += shadow_opacity*texture2D(image, vTexCoord-shadow_dist+(vec2(x, y)/resolution)).a*divider;
				}
			}
		}

		vec4 shadow_blend = (shadowed_px*(1.0-master_px.a));

		vec4 composition = mix(shadow_blend, master_px, master_px.a);

		gl_FragColor = composition;
	} else {
		gl_FragColor = master_px;
	}	
}