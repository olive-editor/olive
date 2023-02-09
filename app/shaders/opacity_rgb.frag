// Inputs
uniform sampler2D tex_in;
uniform sampler2D opacity_in;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

vec3 rgb2hsv(vec3 c)
{
  vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

  float d = q.x - min(q.w, q.y);
  float e = 1.0e-10;
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

void main() {
  vec4 value = texture(opacity_in, ove_texcoord);
  float v = rgb2hsv(value.rgb).b;

  vec4 c = texture(tex_in, ove_texcoord);
  c *= v;
  frag_color = c;
}
