shader videoinput(string footage_in = "" [[ int lockgeom=0 ]],
                   float s = u [[ int lockgeom=0 ]],
                   float t = v [[ int lockgeom=0 ]],
                   output color Cout = 0)
{
    Cout = texture (footage_in, s, t);
}

/*#version 110

varying vec2 ove_texcoord;
uniform sampler2D footage_in;

void main(void) {
  gl_FragColor = texture2D(footage_in, ove_texcoord);
}*/
