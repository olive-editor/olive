uniform sampler2D tex_in;
in vec2 ove_texcoord;
out vec4 frag_color;

// OCIO shader code
%1

// Assume D65 white point
float Xn = 95.0489;
float Yn = 100.0;
float Zn = 108.8840;
float delta = 0.20689655172; // 6/29

float func(float t) {
  if (t > pow(delta, 3.0)){
    return pow(t, 1.0/3.0);
  } else{
    return (t / (3.0 * pow(delta, 2))) + 4.0/29.0;
  }
}

void main() {
  vec4 col = texture2D(tex_in, ove_texcoord);
  vec4 new_col;

  new_col = %2(col);

  vec4 lab;
  lab.r = 116.0 * func(col.g / Yn) - 16.0;
  lab.g = 500.0 * (func(col.r / Xn) -  func(col.g / Yn));
  lab.b = 200.0 * (func(col.g / Yn) -  func(col.b / Zn));
  lab.w = 1.0;
  frag_color = col;
}
