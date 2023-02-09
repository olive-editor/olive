uniform float frequency_in;
uniform float intensity_in;
uniform float evolution_in;
uniform bool vertical_in;

uniform sampler2D tex_in;

in vec2 ove_texcoord;
out vec4 frag_color;

void main(void) {
  vec2 pos = ove_texcoord;

  if (vertical_in) {
    pos.x -= sin((ove_texcoord.y-(evolution_in*0.01))*frequency_in)*intensity_in*0.01;
  } else {
    pos.y -= sin((ove_texcoord.x-(evolution_in*0.01))*frequency_in)*intensity_in*0.01;
  }

  if (pos.x < 0.0 || pos.x >= 1.0 || pos.y < 0.0 || pos.y >= 1.0) {
    discard;
  } else {
    frag_color = texture(tex_in, pos);
  }
}
