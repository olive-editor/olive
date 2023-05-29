// Main texture input
uniform sampler2D ove_maintex;
uniform int ove_maintex_alpha;
uniform mat4 ove_cropmatrix;
uniform bool ove_force_opaque;

// Macros defining `ove_maintex_alpha` state
// Matches `AlphaAssociated` C++ enum
#define ALPHA_NONE     0
#define ALPHA_UNASSOC  1
#define ALPHA_ASSOC    2

// Main texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

// Program will replace this with OCIO's auto-generated shader code
%1

// Alpha association functions
vec4 assoc(vec4 c) {
  return vec4(c.rgb * c.a, c.a);
}

vec4 reassoc(vec4 c) {
  return (c.a == 0.0) ? c : assoc(c);
}

vec4 deassoc(vec4 c) {
  return (c.a == 0.0) ? c : vec4(c.rgb / c.a, c.a);
}

void main() {
  vec2 cropped_coord = (vec4(ove_texcoord-vec2(0.5, 0.5), 0.0, 1.0)*ove_cropmatrix).xy + vec2(0.5, 0.5);
  if (cropped_coord.x < 0.0 || cropped_coord.x >= 1.0 || cropped_coord.y < 0.0 || cropped_coord.y >= 1.0) {
    frag_color = vec4(0.0);
    return;
  }

  vec4 col = texture(ove_maintex, cropped_coord);

  // If alpha is associated, de-associate now
  if (ove_maintex_alpha == ALPHA_ASSOC) {
    col = deassoc(col);
  }

  // Perform color conversion
  col = OCIODisplay(col);

  // Associate or re-associate here
  if (ove_maintex_alpha == ALPHA_ASSOC) {
    col = reassoc(col);
  } else if (ove_maintex_alpha == ALPHA_UNASSOC) {
    col = assoc(col);
  }

  if (ove_force_opaque) {
    col.a = 1.0;
  }

  frag_color = col;
}
