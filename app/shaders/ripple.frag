uniform float evolution_in;
uniform float intensity_in;
uniform float frequency_in;
uniform vec2 position_in;
uniform bool stretch_in;

uniform vec2 resolution_in;
uniform sampler2D tex_in;

in vec2 ove_texcoord;
out vec4 frag_color;

void main(void) {
  vec2 center = position_in/resolution_in;

  vec2 adj_texcoord = ove_texcoord;

  adj_texcoord -= 0.5;
  if (!stretch_in) {
    // Adjust by aspect ratio
    float ar = (resolution_in.x/resolution_in.y);
    if (resolution_in.x > resolution_in.y) {
      adj_texcoord.y /= ar;
      center.y /= ar;
    } else {
      adj_texcoord.x *= ar;
      center.x *= ar;
    }
  }
  adj_texcoord += 0.5;
  center += 0.5;

  adj_texcoord -= center;

  float len = length(adj_texcoord);
  vec2 uv = ove_texcoord + (adj_texcoord/len)*cos((frequency_in)*(len*12.0-evolution_in))*(intensity_in*0.0005);
  frag_color = texture(tex_in, uv);
}
