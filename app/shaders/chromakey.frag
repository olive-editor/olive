// Main texture input
uniform sampler2D tex_in;
uniform vec4 color_key;
uniform bool mask_only_in;
uniform float upper_tolerence_in;
uniform float lower_tolerence_in;

uniform sampler2D garbage_in;
uniform sampler2D core_in;
uniform bool garbage_in_enabled;
uniform bool core_in_enabled;
uniform bool invert_in;

uniform float highlights_in;
uniform float shadows_in;


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

float colorclose(vec4 col, vec4 key, float tola,float tolb) { 
  // Decides if a color is close to the specified hue
  float temp = sqrt(((key.g-col.g)*(key.g-col.g))+((key.b-col.b)*(key.b-col.b)));
  if (temp < tola) {return (0.0);} 
  if (temp < tolb) {return ((temp-tola)/(tolb-tola));} 
  return (1.0); 
}


void main() {

  vec4 col = texture(tex_in, ove_texcoord);

  vec4 unassoc = col;
  if (unassoc.a > 0) {
    unassoc.rgb /= unassoc.a;
  }

  // Perform color conversion
  vec4 cie_xyz = SceneLinearToCIEXYZ_d65(unassoc);
  vec4 lab = CIExyz_to_Lab(cie_xyz);

  vec4 cie_xyz_key = SceneLinearToCIEXYZ_d65(color_key);
  vec4 lab_key = CIExyz_to_Lab(cie_xyz_key);

  float mask = colorclose(lab, lab_key, lower_tolerence_in, upper_tolerence_in);

  mask = clamp(mask, 0.0, 1.0);

  if (garbage_in_enabled) {
    // Force anything we want to remove to be 0.0
    vec4 garbage = texture(garbage_in, ove_texcoord);
    // Assumes garbage is achromatic
    mask -= garbage.r;
    mask = clamp(mask, 0.0, 1.0);
  }

  if (core_in_enabled) {
    // Force anything we want to keep to be 1.0
    vec3 core = texture(core_in, ove_texcoord).rgb;
    // Assumes core is achromatic
    mask += core.r;
    mask = clamp(mask, 0.0, 1.0);
  }

  // Crush blacks and push whites
  mask = shadows_in * 0.01 * (highlights_in * 0.01 * mask - 1.0) + 1.0;
  mask = clamp(mask, 0.0, 1.0);

  // Invert
  if (invert_in) {
    mask = 1.0 - mask;
  }

  col.rgb *= mask;
  col.w = mask;

  if (!mask_only_in) {
    frag_color = col;
  } else {
    frag_color = vec4(vec3(mask), 1.0);
  }
}
