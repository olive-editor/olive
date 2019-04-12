/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "crossdissolvetransition.h"

#include <QOpenGLFunctions>

CrossDissolveTransition::CrossDissolveTransition(Clip* c) : Transition(c) {
  SetFlags(Node::CoordsFlag);
}

QString CrossDissolveTransition::name()
{
  return tr("Cross Dissolve");
}

QString CrossDissolveTransition::id()
{
  return "org.olivevideoeditor.Olive.crossdissolve";
}

QString CrossDissolveTransition::category()
{
  return tr("Dissolves");
}

QString CrossDissolveTransition::description()
{
  return tr("Dissolve clips evenly.");
}

EffectType CrossDissolveTransition::type()
{
  return EFFECT_TYPE_TRANSITION;
}

olive::TrackType CrossDissolveTransition::subtype()
{
  return olive::kTypeVideo;
}

NodePtr CrossDissolveTransition::Create(Clip *c)
{
  return std::make_shared<CrossDissolveTransition>(c);
}

void CrossDissolveTransition::process_coords(double progress, GLTextureCoords& coords, int data) {
  if (data == kTransitionClosing) {
    coords.opacity *= (1.0 - progress);
  } else {
    coords.opacity *= progress;
  }
}
