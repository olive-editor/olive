// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

// Match with ShapeNode::Type
const int SHAPE_RECTANGLE = 0;
const int SHAPE_ELLIPSE = 1;
const int SHAPE_ROUNDEDRECT = 2;

uniform vec2 pos_in;
uniform vec2 size_in;
uniform int type_in;
uniform vec2 resolution_in;
uniform vec4 color_in;
uniform float radius_in;

vec4 draw_rect(vec2 real_position, vec2 real_size)
{
  if (ove_texcoord.x >= real_position.x && ove_texcoord.y >= real_position.y
      && ove_texcoord.x < real_position.x+real_size.x && ove_texcoord.y < real_position.y+real_size.y) {
    return color_in;
  } else {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }
}

vec4 draw_ellipse(vec2 center, float radius, float aspect_ratio) {
  vec2 offset = ove_texcoord*resolution_in - center;
  offset.x /= aspect_ratio;
  float d = length(offset)-radius;
  float t = clamp(d, 0.0, 1.0);

  return color_in * (1.0-t);
}

void main() {
  vec2 p = pos_in + resolution_in*0.5 - size_in*0.5;

  vec2 real_position = p/resolution_in;
  vec2 real_size = size_in/resolution_in;

  vec4 col = vec4(0.0);

  switch (type_in) {
  case SHAPE_RECTANGLE:
  {
    col = draw_rect(real_position, real_size);
    break;
  }
  case SHAPE_ELLIPSE:
  {
    vec2 center = p+size_in*0.5;
    float radius = size_in.y*0.5;
    float aspect_ratio = size_in.x/size_in.y;
    col = draw_ellipse(center, radius, aspect_ratio);
    break;
  }
  case SHAPE_ROUNDEDRECT:
  {
    // Limit radius so it is never larger than half the shortest size
    float r = min(radius_in, min(size_in.y*0.5, size_in.x*0.5));
    vec2 real_rad = vec2(r / resolution_in.x, r / resolution_in.y);
    if (ove_texcoord.x < real_position.x + real_rad.x && ove_texcoord.y < real_position.y + real_rad.y) {
      // Top-left
      col = draw_ellipse(p + r, r, 1.0);
    } else if (ove_texcoord.x > real_position.x+real_size.x - real_rad.x && ove_texcoord.y < real_position.y + real_rad.y) {
      // Top-right
      col = draw_ellipse(vec2(p.x + size_in.x - r, p.y + r), r, 1.0);
    } else if (ove_texcoord.x < real_position.x + real_rad.x && ove_texcoord.y > real_position.y + real_size.y - real_rad.y) {
      // Bottom-left
      col = draw_ellipse(vec2(p.x + r, p.y + size_in.y - r), r, 1.0);
    } else if (ove_texcoord.x > real_position.x+real_size.x - real_rad.x && ove_texcoord.y > real_position.y + real_size.y - real_rad.y) {
      // Bottom-right
      col = draw_ellipse(vec2(p.x + size_in.x - r, p.y + size_in.y - r), r, 1.0);
    } else {
      col = draw_rect(real_position, real_size);
    }
    break;
  }
  }

  frag_color = col;
}
