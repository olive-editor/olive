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

#include "paneffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QtMath>
#include <cmath>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"

PanEffect::PanEffect(Clip* c, const EffectMeta *em) : Effect(c, em) {
  pan_val = new DoubleInput(this, "pan", tr("Pan"));
  pan_val->SetMinimum(-100);
  pan_val->SetDefault(0);
  pan_val->SetMaximum(100);
}

void PanEffect::process_audio(double timecode_start,
                              double timecode_end,
                              float **samples,
                              int nb_samples,
                              int channel_count,
                              int type) {

  Q_UNUSED(type)

  // This has no effect on mono sources
  if (channel_count < 2) {
    return;
  }

  double interval = (timecode_end - timecode_start)/nb_samples;

  for (int i=0;i<nb_samples;i++) {
    double pan_field_val = pan_val->GetDoubleAt(timecode_start+(interval*i));
    double pval = log_volume(qAbs(pan_field_val)*0.01);

    if (pan_field_val < 0) {
      // affect right channel
      samples[1][i] *= (1.0-pval);
    } else {
      // affect left channel
      samples[0][i] *= (1.0-pval);
    }
  }
}
