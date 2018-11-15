#version 110

uniform float amount;

uniform sampler2D tex0;
uniform vec2 resolution; // screen resolution
#define T texture2D(tex0,.5+(p.xy*=.992))
void main() 
{
  vec3 p = gl_FragCoord.xyz/vec3(resolution, 1.0)-.5;
  vec3 o = T.rgb;
  for (float i=0.;i<amount;i++) 
    p.z += pow(max(0.,.5-length(T.rg)),2.)*exp(-i*.08);
  gl_FragColor=vec4(o*o+p.z,1);
}