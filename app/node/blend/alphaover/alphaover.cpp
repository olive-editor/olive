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

Node *AlphaOverBlend::copy() const
{
  return new AlphaOverBlend();
}

QString AlphaOverBlend::Name() const
{
  return tr("Alpha Over");
}

QString AlphaOverBlend::id() const
{
  return "org.olivevideoeditor.Olive.alphaoverblend";
}

QString AlphaOverBlend::Description() const
{
  return tr("A blending node that composites one texture over another using its alpha channel.");
}

bool AlphaOverBlend::IsAccelerated() const
{
  return true;
}

QString AlphaOverBlend::CodeFragment() const
{
  return ReadFileAsString(":/shaders/alphaover.frag");
}
