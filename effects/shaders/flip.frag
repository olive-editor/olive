uniform bool horiz;
uniform bool vert;

vec4 process(vec4 col) {
	float x = v_texcoord.x;
	float y = v_texcoord.y;

	if (!horiz && !vert) return col;

	if (horiz) x = 1.0 - x;
	if (vert) y = 1.0 - y;

	return texture2D(texture, vec2(x, y));
}