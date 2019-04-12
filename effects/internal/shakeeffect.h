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

#ifndef SHAKEEFFECT_H
#define SHAKEEFFECT_H

#include "nodes/node.h"

#define RANDOM_VAL_SIZE 30

class ShakeEffect : public Node {
  Q_OBJECT
public:
  ShakeEffect(Clip* c);
  virtual void process_coords(double timecode, GLTextureCoords& coords, int data) override;

  virtual QString name() override;
  virtual QString id() override;
  virtual QString category() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual NodePtr Create(Clip *c) override;

  DoubleInput* intensity_val;
  DoubleInput* rotation_val;
  DoubleInput* frequency_val;
private:
  double random_vals[RANDOM_VAL_SIZE];
};

#endif // SHAKEEFFECT_H
