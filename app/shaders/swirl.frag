// Swirl effect parameters
uniform float radius_in;
uniform float angle_in;
uniform vec2 pos_in;
uniform vec2 resolution_in;

uniform sampler2D tex_in;

in vec2 ove_texcoord;
out vec4 frag_color;

void main(void) {
  vec2 center = resolution_in*0.5 + pos_in;

  vec2 uv = ove_texcoord;

  vec2 tc = uv * resolution_in;
  tc -= center;
  float dist = length(tc);
  if (dist < radius_in) {
    float percent = (radius_in - dist) / radius_in;
    float theta = percent * percent * -angle_in;
    float s = sin(theta);
    float c = cos(theta);
    tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));
  }
  tc += center;
  frag_color = texture(tex_in, tc / resolution_in);
}
