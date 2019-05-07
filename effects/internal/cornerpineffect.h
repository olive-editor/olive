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

#ifndef CORNERPINEFFECT_H
#define CORNERPINEFFECT_H

#include "nodes/oldeffectnode.h"

class CornerPinEffect : public OldEffectNode {
  Q_OBJECT
public:
  CornerPinEffect(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual OldEffectNodePtr Create(Clip *c) override;

  void process_coords(double timecode, GLTextureCoords& coords, int data);
  void process_shader(double timecode, GLTextureCoords& coords, int iterations);
  void gizmo_draw(double timecode, GLTextureCoords& coords);
private:
  Vec2Input* top_left;
  Vec2Input* top_right;
  Vec2Input* bottom_left;
  Vec2Input* bottom_right;

  BoolInput* perspective;

  EffectGizmo* top_left_gizmo;
  EffectGizmo* top_right_gizmo;
  EffectGizmo* bottom_left_gizmo;
  EffectGizmo* bottom_right_gizmo;
};

#endif // CORNERPINEFFECT_H
