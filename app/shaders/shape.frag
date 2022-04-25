// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

// Match with ShapeNode::Type
#define SHAPE_RECTANGLE 0
#define SHAPE_ELLIPSE 1

uniform vec2 pos_in;
uniform vec2 size_in;
uniform int type_in;
uniform vec2 resolution_in;
uniform vec4 color_in;

void main() {
  vec2 p = pos_in + resolution_in*0.5 - size_in*0.5;

  vec2 real_position = p/resolution_in;
  vec2 real_size = size_in/resolution_in;

  vec4 col = vec4(0.0);

  if (type_in == SHAPE_RECTANGLE) {
    if (ove_texcoord.x >= real_position.x && ove_texcoord.y >= real_position.y
        && ove_texcoord.x < real_position.x+real_size.x && ove_texcoord.y < real_position.y+real_size.y) {
      col = color_in;
    }
  } else if (type_in == SHAPE_ELLIPSE) {
    vec2 center = p+size_in*0.5;
    float radius = size_in.y*0.5;
    float aspect_ratio = size_in.x/size_in.y;

    vec2 offset = ove_texcoord*resolution_in - center;
    offset.x /= aspect_ratio;
    float d = length(offset)-radius;
    float t = clamp(d, 0.0, 1.0);

    col = color_in * (1.0-t);
  }

  frag_color = col;
}
