// Input texture
uniform sampler2D tex_in;
uniform vec3 custom_weights_in;
uniform bool custom_weights_enable_in;
uniform vec3 luma_coeffs;

// Input texture coordinate
in vec2 ove_texcoord;
out vec4 frag_color;

void main() {
  vec4 color = texture(tex_in, ove_texcoord);
  if (!custom_weights_enable_in){
    frag_color = vec4(vec3(dot(color.rgb, luma_coeffs)), color.w);
  } else {
    frag_color = vec4(vec3(dot(color.rgb, custom_weights_in)), color.w);
  }
}
