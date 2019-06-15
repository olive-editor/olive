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

#ifndef TONEEFFECT_H
#define TONEEFFECT_H

#include "nodes/oldeffectnode.h"

class ToneEffect : public OldEffectNode {
  Q_OBJECT
public:
  ToneEffect(Clip* c);

  virtual QString name() override;
  virtual QString id() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual OldEffectNodePtr Create(Clip *c) override;

  virtual void process_audio(double timecode_start,
                             double timecode_end,
                             float **samples,
                             int nb_samples,
                             int channel_count,
                             int type) override;

private:
  ComboInput* type_val;
  DoubleInput* freq_val;
  DoubleInput* amount_val;
  BoolInput* mix_val;

  int sinX;
};

#endif // TONEEFFECT_H
