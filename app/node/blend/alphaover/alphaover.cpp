/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "alphaover.h"

AlphaOverBlend::AlphaOverBlend()
{

}

QString AlphaOverBlend::Name()
{
  return tr("Alpha Over");
}

QString AlphaOverBlend::id()
{
  return "org.olivevideoeditor.Olive.alphaoverblend";
}

QString AlphaOverBlend::Description()
{
  return tr("A blending node that composites one texture over another using its alpha channel.");
}

QString AlphaOverBlend::Code(NodeOutput *output)
{
  if (output == texture_output()) {
    return "#version 110"
           "\n"
           "varying vec2 olive_tex_coord;\n"
           "\n"
           "uniform sampler2D base_in;\n"
           "uniform sampler2D blend_in;\n"
           "\n"
           "void main(void) {\n"
           "  gl_FragColor = base_in - blend_in.a + blend_in;\n"
           "}\n";
  }

  return Node::Code(output);
}
