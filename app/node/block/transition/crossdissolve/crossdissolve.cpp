#include "crossdissolve.h"

CrossDissolveTransition::CrossDissolveTransition()
{

}

Node *CrossDissolveTransition::copy() const
{
  return new CrossDissolveTransition();
}

QString CrossDissolveTransition::Name() const
{
  return tr("Cross Dissolve");
}

QString CrossDissolveTransition::id() const
{
  return "org.olivevideoeditor.Olive.crossdissolve";
}

QString CrossDissolveTransition::Description() const
{
  return tr("A smooth fade transition from one video clip to another.");
}

bool CrossDissolveTransition::IsAccelerated() const
{
  return true;
}

QString CrossDissolveTransition::CodeFragment() const
{
  return "#version 110\n"
         "\n"
         "varying vec2 ove_texcoord;\n"
         "\n"
         "uniform sampler2D out_block_in;\n"
         "uniform sampler2D in_block_in;\n"
         "\n"
         "uniform float transition_total_prog;\n"
         "uniform float transition_out_prog;\n"
         "uniform float transition_in_prog;\n"
         "\n"
         "void main(void) {\n"
         "  vec4 out_tex = texture2D(out_block_in, ove_texcoord);\n"
         "  vec4 in_tex = texture2D(in_block_in, ove_texcoord);\n"
         "  \n"
         "  in_tex *= transition_total_prog;\n"
         "  \n"
         "  gl_FragColor = out_tex - in_tex.a + in_tex;\n"
         "}\n";
}
