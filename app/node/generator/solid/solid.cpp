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

#include "solid.h"

SolidGenerator::SolidGenerator() :
  texture_(nullptr)
{
  color_input_ = new NodeInput("color_in");
  color_input_->set_data_type(NodeParam::kColor);
  AddParameter(color_input_);
}

Node *SolidGenerator::copy() const
{
  return new SolidGenerator();
}

QString SolidGenerator::Name() const
{
  return tr("Solid");
}

QString SolidGenerator::id() const
{
  return "org.olivevideoeditor.Olive.solidgenerator";
}

QString SolidGenerator::Category() const
{
  return tr("Generator");
}

QString SolidGenerator::Description() const
{
  return tr("Generate a solid color.");
}

QString SolidGenerator::Code() const
{
  // FIXME: Not color managed
  return "#version 110\n"
         "\n"
         "uniform vec4 color_in;\n"
         "\n"
         "void main(void) {\n"
         "  gl_FragColor = color_in;\n"
         "}\n";
}
