// Main texture input
uniform sampler2D tex_in;


// Main texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

// Program will replace this with OCIO's auto-generated shader code
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

vec4 CIExyz_to_Lab(vec4 CIE) {
  vec4 lab;
  lab.r = 116.0 * func(CIE.g / Yn) - 16.0;
  lab.g = 500.0 * (func(CIE.r / Xn) -  func(CIE.g / Yn));
  lab.b = 200.0 * (func(CIE.g / Yn) -  func(CIE.b / Zn));
  lab.w = CIE.w;

  return lab;
}


void main() {

  vec4 col = texture(tex_in, ove_texcoord);

  // Perform color conversion
  vec4 cie_xyz = SceneLinearToCIEXYZ_d65(col);
  vec4 lab = CIExyz_to_Lab(cie_xyz);

  frag_color = vec4(lab.r);
}
