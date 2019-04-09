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

#include "volumeeffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <stdint.h>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

VolumeEffect::VolumeEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
  EffectRow* volume_row = new EffectRow(this, tr("Volume"));
  volume_val = new DoubleField(volume_row, "volume");

  // set defaults
  volume_val->SetDefault(1);
  volume_val->SetDisplayType(LabelSlider::Decibel);
}

void VolumeEffect::process_audio(double timecode_start,
                                 double timecode_end,
                                 float **samples,
                                 int nb_samples,
                                 int channel_count,
                                 int type) {

  Q_UNUSED(type)

  double interval = (timecode_end-timecode_start)/nb_samples;

  for (int i=0;i<nb_samples;i++) {

    double vol_val = volume_val->GetDoubleAt(timecode_start+(interval*i));

    for (int j=0;j<channel_count;j++) {

      samples[j][i] *= vol_val;

    }
  }
}
