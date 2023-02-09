uniform float scale_in;
uniform vec2 position_in;
uniform vec2 resolution_in;
uniform bool mirrorx_in;
uniform bool mirrory_in;
uniform int anchor_in;

uniform sampler2D tex_in;

in vec2 ove_texcoord;
out vec4 frag_color;

#define TOP_LEFT        0
#define TOP_CENTER      1
#define TOP_RIGHT       2
#define MIDDLE_LEFT     3
#define MIDDLE_CENTER   4
#define MIDDLE_RIGHT    5
#define BOTTOM_LEFT     6
#define BOTTOM_CENTER   7
#define BOTTOM_RIGHT    8

void main(void) {
  vec2 coord = ove_texcoord;

  vec2 offset;

  if (anchor_in == TOP_LEFT || anchor_in == TOP_CENTER || anchor_in == TOP_RIGHT) {
    offset.y = 0.0;
  } else if (anchor_in == MIDDLE_LEFT || anchor_in == MIDDLE_CENTER || anchor_in == MIDDLE_RIGHT) {
    offset.y = 0.5;
  } else if (anchor_in == BOTTOM_LEFT || anchor_in == BOTTOM_CENTER || anchor_in == BOTTOM_RIGHT) {
    offset.y = 1.0;
  }

  if (anchor_in == TOP_LEFT || anchor_in == MIDDLE_LEFT || anchor_in == BOTTOM_LEFT) {
    offset.x = 0.0;
  } else if (anchor_in == TOP_CENTER || anchor_in == MIDDLE_CENTER || anchor_in == BOTTOM_CENTER) {
    offset.x = 0.5;
  } else if (anchor_in == TOP_RIGHT || anchor_in == MIDDLE_RIGHT || anchor_in == BOTTOM_RIGHT) {
    offset.x = 1.0;
  }

  coord -= position_in/resolution_in;

  coord -= offset;
  coord /= scale_in;
  coord += offset;

  vec2 modcoord = mod(coord, 1.0);

  if (mirrorx_in && mod(coord.x, 2.0) > 1.0) {
    modcoord.x = 1.0 - modcoord.x;
  }

  if (mirrory_in && mod(coord.y, 2.0) > 1.0) {
    modcoord.y = 1.0 - modcoord.y;
  }

  frag_color = vec4(texture(tex_in, modcoord));
}
