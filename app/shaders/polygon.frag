uniform vec2[256] points_in;
uniform int points_in_count;
uniform vec4 color_in;

uniform vec2 resolution_in;

in vec2 ove_texcoord;

out vec4 fragColor;

/*
int pnpoly(int npol, float *xp, float *yp, float x, float y) {
  int i, j, c = 0;
  for (i = 0, j = npol-1; i < npol; j = i++) {
    if ((((yp[i] <= y) && (y < yp[j])) ||
         ((yp[j] <= y) && (y < yp[i]))) &&
        (x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))
      c = !c;
  }
  return c;
}
*/

bool pnpoly(vec2 p) {
  bool c = false;
  int i, j;
  for (i = 0, j = points_in_count-1; i < points_in_count; j = i++) {
    if ((((points_in[i].y <= p.y) && (p.y < points_in[j].y)) ||
         ((points_in[j].y <= p.y) && (p.y < points_in[i].y))) &&
        (p.x < (points_in[j].x - points_in[i].x) * (p.y - points_in[i].y) / (points_in[j].y - points_in[i].y) + points_in[i].x))
      c = !c;
  }
  return c;
}

void main(void) {
    if (points_in_count > 0 && pnpoly(ove_texcoord * resolution_in)) {
        fragColor = color_in;
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
