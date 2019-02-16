#version 110

uniform sampler2D tex1;
uniform sampler3D tex2;

void main()
{
    vec4 col = texture2D(tex1, gl_TexCoord[0].st);
    gl_FragColor = OCIODisplay(col, tex2);
}