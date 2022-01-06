uniform sampler2D ove_maintex;

uniform vec2 resolution_in;

varying vec2 ove_texcoord;

float round(float x)
{
  return floor(x + 0.5);
}

void main() {
    vec2 using_texcoord = ove_texcoord;

    // A very basic deinterlace that halves the vertical
    // resolution and linearly interpolates the two fields
    // by reading the texture coord between them.
    float half_vert = round(resolution_in.y / 2.0);
    using_texcoord.y = (round(using_texcoord.y * half_vert) + 0.25) / half_vert;

    vec4 color = texture2D(ove_maintex, using_texcoord);
    gl_FragColor = color;
}
