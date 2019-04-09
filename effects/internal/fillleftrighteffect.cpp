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

#include "fillleftrighteffect.h"

enum FillType {
  FILL_TYPE_LEFT,
  FILL_TYPE_RIGHT
};

FillLeftRightEffect::FillLeftRightEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
  EffectRow* type_row = new EffectRow(this, tr("Type"));
  fill_type = new ComboField(type_row, "type");
  fill_type->AddItem(tr("Fill Left with Right"), FILL_TYPE_LEFT);
  fill_type->AddItem(tr("Fill Right with Left"), FILL_TYPE_RIGHT);
}

void FillLeftRightEffect::process_audio(double timecode_start,
                                        double timecode_end,
                                        float **samples,
                                        int nb_samples,
                                        int channel_count,
                                        int type) {

  Q_UNUSED(type)

  double interval = (timecode_end-timecode_start)/nb_samples;

  if (channel_count == 2) {
    for (int i=0;i<nb_samples;i++) {
      if (fill_type->GetValueAt(timecode_start+(interval*i)) == FILL_TYPE_LEFT) {
        samples[0][i] = samples[1][i];
      } else {
        samples[1][i] = samples[0][i];
      }
    }
  }
}
