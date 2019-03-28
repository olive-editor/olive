uniform float left;
uniform float top;
uniform float right;
uniform float bottom;
uniform float feather;

uniform mediump float amount_val;

vec4 process(vec4 col) {
	float alpha = 1.0;


	if (feather == 0.0) {
		if (v_texcoord.x < (left*0.01) || v_texcoord.y < (top*0.01) || v_texcoord.x > (1.0-(right*0.01)) || v_texcoord.y > (1.0-(bottom*0.01))) {
			alpha = 0.0;
		}
	} else {
		float f = pow(2.0, 10.0-(feather*0.1));

		if (left > 0.0) {
			alpha *= clamp(((v_texcoord.x+(0.5/f))-(left*0.01))*f, 0.0, 1.0); // left
		}

		if (top > 0.0) {
			alpha *= clamp(((v_texcoord.y+(0.5/f))-(top*0.01))*f, 0.0, 1.0); // top
		}

		if (right > 0.0) {
			alpha *= clamp((((1.0-v_texcoord.x)+(0.5/f))-(right*0.01))*f, 0.0, 1.0); // right
		}

		if (bottom > 0.0) {
			alpha *= clamp((((1.0-v_texcoord.y)+(0.5/f))-(bottom*0.01))*f, 0.0, 1.0); // bottom
		}
	}

	return col * alpha;
}