// Input texture
uniform sampler2D tex_in;

uniform float horiz_in;
uniform float vert_in;

// Input texture coordinate
in vec2 ove_texcoord;

// Output color
out vec4 fragColor;

void main() {
  float x;
  float y;

  if (horiz_in > 0.0) {
    x = floor(ove_texcoord.x * horiz_in) / horiz_in;
  } else {
    x = ove_texcoord.x;
  }

  if (vert_in > 0.0) {
    y = floor(ove_texcoord.y * vert_in) / vert_in;
  } else {
    y = ove_texcoord.y;
  }

  vec4 color = texture(tex_in, vec2(x, y));
  fragColor = color;
}
